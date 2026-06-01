#pragma once

#include <string>
#include <unordered_map>
#include <thread>
#include <memory>

namespace solo
{

class S_ResourceData
{
public:
    S_ResourceData(const std::string &name, const size_t &size, const unsigned char *data );

    std::string name() const;
    size_t size() const;
    const unsigned char *data() const;

private:
    std::string m_name;
    size_t   m_size;
    const unsigned char *m_data;
};

class S_ResourcesContainer
{
public:
    S_ResourcesContainer();
    const std::unordered_map<std::string, std::unique_ptr<S_ResourceData>>* resources() const;

private:
    void updateResources(size_t numberOfResources, const unsigned char *fileNames, const size_t *fileSizes, const unsigned char *filesData );
    std::unordered_map<std::string, std::unique_ptr<S_ResourceData>> m_resources;

};

}

#include "S_Resources.inl"
