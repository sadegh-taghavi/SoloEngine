#pragma once
#include <cstdint>
#include <limits>

namespace solo
{

class S_MaterialPool
{
public:
    static constexpr uint32_t Invalid = std::numeric_limits<uint32_t>::max();

    uint32_t allocate() { return m_next++; }
    uint32_t count()    const { return m_next; }

private:
    uint32_t m_next = 0;
};

}
