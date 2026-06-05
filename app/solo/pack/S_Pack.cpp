#include "S_Pack.h"
#include "solo/debug/S_Debug.h"
#include <cstring>

namespace solo
{

#pragma pack(push, 1)
struct PackHeader { uint32_t magic; uint32_t version; uint32_t count; uint32_t reserved; };
struct PackEntry  { char name[256]; uint64_t offset; uint64_t size; };
#pragma pack(pop)

static constexpr uint32_t PACK_MAGIC = 0x4b415053;

bool S_Pack::open(const std::string& path)
{
    m_file.open(path, std::ios::binary);
    if (!m_file) { s_debugLayer("S_Pack: failed to open", path); return false; }
    PackHeader header{};
    m_file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.magic != PACK_MAGIC) { s_debugLayer("S_Pack: invalid magic in", path); return false; }
    for (uint32_t i = 0; i < header.count; ++i)
    {
        PackEntry e{};
        m_file.read(reinterpret_cast<char*>(&e), sizeof(e));
        m_toc[e.name] = { e.offset, e.size };
    }
    s_debugLayer("S_Pack: opened", path, "—", m_toc.size(), "entries");
    return true;
}

void S_Pack::close()
{
    m_file.close();
    m_toc.clear();
}

std::vector<uint8_t> S_Pack::load(const std::string& name) const
{
    auto it = m_toc.find(name);
    if (it == m_toc.end()) return {};
    std::vector<uint8_t> data(it->second.size);
    m_file.seekg(static_cast<std::streamoff>(it->second.offset));
    m_file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(it->second.size));
    return data;
}

bool S_Pack::exists(const std::string& name) const
{
    return m_toc.find(name) != m_toc.end();
}

}
