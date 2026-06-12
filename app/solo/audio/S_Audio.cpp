#include "S_Audio.h"
#include "solo/application/S_Application.h"
#include "solo/pack/S_Pack.h"
#include "solo/debug/S_Debug.h"

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include <miniaudio.h>

#include <vector>
#include <cstring>

using namespace solo;

namespace
{

// ma_vfs backed by the resource pack: whole file is loaded into memory on
// open, reads/seeks run against that buffer
struct PackVFS
{
    ma_vfs_callbacks cb; // must be first
};

struct PackFile
{
    std::vector<uint8_t> data;
    size_t               cursor = 0;
};

ma_result packOpen(ma_vfs*, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
{
    if (openMode & MA_OPEN_MODE_WRITE)
        return MA_NOT_IMPLEMENTED;
    auto data = S_Application::executingApplication()->pack()->load(pFilePath);
    if (data.empty())
        return MA_DOES_NOT_EXIST;
    auto* f = new PackFile();
    f->data = std::move(data);
    *pFile = f;
    return MA_SUCCESS;
}

ma_result packClose(ma_vfs*, ma_vfs_file file)
{
    delete static_cast<PackFile*>(file);
    return MA_SUCCESS;
}

ma_result packRead(ma_vfs*, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead)
{
    auto* f = static_cast<PackFile*>(file);
    size_t remaining = f->data.size() - f->cursor;
    size_t toRead    = sizeInBytes < remaining ? sizeInBytes : remaining;
    if (toRead > 0)
    {
        memcpy(pDst, f->data.data() + f->cursor, toRead);
        f->cursor += toRead;
    }
    if (pBytesRead) *pBytesRead = toRead;
    return toRead == 0 && sizeInBytes > 0 ? MA_AT_END : MA_SUCCESS;
}

ma_result packSeek(ma_vfs*, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
{
    auto* f = static_cast<PackFile*>(file);
    ma_int64 base = 0;
    switch (origin)
    {
    case ma_seek_origin_start:   base = 0; break;
    case ma_seek_origin_current: base = static_cast<ma_int64>(f->cursor); break;
    case ma_seek_origin_end:     base = static_cast<ma_int64>(f->data.size()); break;
    }
    ma_int64 target = base + offset;
    if (target < 0 || target > static_cast<ma_int64>(f->data.size()))
        return MA_BAD_SEEK;
    f->cursor = static_cast<size_t>(target);
    return MA_SUCCESS;
}

ma_result packTell(ma_vfs*, ma_vfs_file file, ma_int64* pCursor)
{
    *pCursor = static_cast<ma_int64>(static_cast<PackFile*>(file)->cursor);
    return MA_SUCCESS;
}

ma_result packInfo(ma_vfs*, ma_vfs_file file, ma_file_info* pInfo)
{
    pInfo->sizeInBytes = static_cast<PackFile*>(file)->data.size();
    return MA_SUCCESS;
}

}

struct S_Audio::Impl
{
    static constexpr uint32_t kMaxVoices = 64;

    struct Voice
    {
        ma_sound sound{};
        uint32_t generation = 0;
        bool     active     = false;
        bool     loop       = false;
    };

    PackVFS   vfs{};
    ma_engine engine{};
    bool      initialized = false;
    Voice     voices[kMaxVoices];

    S_SoundHandle startVoice(const std::string& path, bool spatial, const glm::vec3& pos,
                             float volume, bool loop)
    {
        if (!initialized) return {};

        for (uint32_t i = 0; i < kMaxVoices; ++i)
        {
            Voice& v = voices[i];
            if (v.active) continue;

            ma_uint32 flags = MA_SOUND_FLAG_DECODE; // small SFX: decode up front
            if (ma_sound_init_from_file(&engine, path.c_str(), flags, nullptr, nullptr, &v.sound) != MA_SUCCESS)
            {
                s_debugLayer("S_Audio: failed to load", path);
                return {};
            }

            ma_sound_set_spatialization_enabled(&v.sound, spatial ? MA_TRUE : MA_FALSE);
            if (spatial)
            {
                ma_sound_set_position(&v.sound, pos.x, pos.y, pos.z);
                ma_sound_set_min_distance(&v.sound, 5.0f);
                ma_sound_set_attenuation_model(&v.sound, ma_attenuation_model_inverse);
            }
            ma_sound_set_volume(&v.sound, volume);
            ma_sound_set_looping(&v.sound, loop ? MA_TRUE : MA_FALSE);
            ma_sound_start(&v.sound);

            v.active = true;
            v.loop   = loop;
            s_debugLayer("S_Audio: play", path, spatial ? "(3D)" : "(2D)");
            return { i, v.generation };
        }
        return {}; // voice pool exhausted; drop the sound
    }

    Voice* get(S_SoundHandle h)
    {
        if (!h.valid() || h.index >= kMaxVoices) return nullptr;
        Voice& v = voices[h.index];
        return (v.active && v.generation == h.generation) ? &v : nullptr;
    }

    void release(Voice& v)
    {
        ma_sound_uninit(&v.sound);
        v.active = false;
        ++v.generation;
    }
};

S_Audio::S_Audio() : m_impl(std::make_unique<Impl>())
{
    m_impl->vfs.cb.onOpen  = packOpen;
    m_impl->vfs.cb.onOpenW = nullptr;
    m_impl->vfs.cb.onClose = packClose;
    m_impl->vfs.cb.onRead  = packRead;
    m_impl->vfs.cb.onWrite = nullptr;
    m_impl->vfs.cb.onSeek  = packSeek;
    m_impl->vfs.cb.onTell  = packTell;
    m_impl->vfs.cb.onInfo  = packInfo;

    ma_engine_config config = ma_engine_config_init();
    config.pResourceManagerVFS = &m_impl->vfs;
    if (ma_engine_init(&config, &m_impl->engine) == MA_SUCCESS)
    {
        m_impl->initialized = true;
        s_debugLayer("S_Audio: miniaudio initialized,",
                     ma_engine_get_sample_rate(&m_impl->engine), "Hz");
    }
    else
        s_debugLayer("S_Audio: engine init FAILED — audio disabled");
}

S_Audio::~S_Audio()
{
    if (m_impl->initialized)
    {
        for (auto& v : m_impl->voices)
            if (v.active)
                m_impl->release(v);
        ma_engine_uninit(&m_impl->engine);
    }
}

void S_Audio::update()
{
    for (auto& v : m_impl->voices)
        if (v.active && !v.loop && ma_sound_at_end(&v.sound))
            m_impl->release(v);
}

void S_Audio::setListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    if (!m_impl->initialized) return;
    ma_engine_listener_set_position(&m_impl->engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(&m_impl->engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&m_impl->engine, 0, up.x, up.y, up.z);
}

S_SoundHandle S_Audio::play(const std::string& packPath, float volume)
{
    return m_impl->startVoice(packPath, false, glm::vec3(0.0f), volume, false);
}

S_SoundHandle S_Audio::play(const std::string& packPath, const glm::vec3& position,
                            float volume, bool loop)
{
    return m_impl->startVoice(packPath, true, position, volume, loop);
}

void S_Audio::setPosition(S_SoundHandle sound, const glm::vec3& position)
{
    if (auto* v = m_impl->get(sound))
        ma_sound_set_position(&v->sound, position.x, position.y, position.z);
}

void S_Audio::setVolume(S_SoundHandle sound, float volume)
{
    if (auto* v = m_impl->get(sound))
        ma_sound_set_volume(&v->sound, volume);
}

void S_Audio::stop(S_SoundHandle sound)
{
    if (auto* v = m_impl->get(sound))
        m_impl->release(*v);
}

void S_Audio::setMasterVolume(float volume)
{
    if (m_impl->initialized)
        ma_engine_set_volume(&m_impl->engine, volume);
}

uint32_t S_Audio::activeVoices() const
{
    uint32_t n = 0;
    for (const auto& v : m_impl->voices)
        if (v.active) ++n;
    return n;
}
