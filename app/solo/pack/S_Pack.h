#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdint>

namespace solo
{

class S_Pack
{
public:
    bool open(const std::string& path);
    void close();
    std::vector<uint8_t> load(const std::string& name) const;
    bool exists(const std::string& name) const;

private:
    struct Entry { uint64_t offset; uint64_t size; };
    mutable std::ifstream m_file;
    std::unordered_map<std::string, Entry> m_toc;
};

}
