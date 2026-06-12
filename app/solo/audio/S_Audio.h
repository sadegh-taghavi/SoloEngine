#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "solo/math/S_Math.h"

namespace solo
{

struct S_SoundHandle
{
    uint32_t index      = 0xFFFFFFFFu;
    uint32_t generation = 0;
    bool valid() const { return index != 0xFFFFFFFFu; }
};

// miniaudio-backed audio system. Sounds load from the resource pack through a
// custom VFS (same paths as every other asset, e.g. "sounds/click.wav").
// 3D sounds use miniaudio's spatializer: amplitude panning + distance
// attenuation + doppler, listener driven from the camera each frame.
class S_Audio
{
public:
    S_Audio();
    ~S_Audio();

    void update(); // reclaims finished one-shot voices; call once per frame

    void setListener(const glm::vec3& position, const glm::vec3& forward,
                     const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    // 2D (UI) sound — no spatialization
    S_SoundHandle play(const std::string& packPath, float volume = 1.0f);
    // positioned 3D sound
    S_SoundHandle play(const std::string& packPath, const glm::vec3& position,
                       float volume = 1.0f, bool loop = false);

    void setPosition(S_SoundHandle sound, const glm::vec3& position);
    void setVolume(S_SoundHandle sound, float volume);
    void stop(S_SoundHandle sound);

    void setMasterVolume(float volume);
    uint32_t activeVoices() const;

private:
    struct Impl; // hides miniaudio from engine headers
    std::unique_ptr<Impl> m_impl;
};

}
