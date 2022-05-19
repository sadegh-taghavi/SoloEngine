#pragma once

#include "solo/stl/S_String.h"
#include "solo/stl/S_UnorderedMap.h"
#include <thread>
#include <memory>

namespace solo
{

class S_ResourceData
{
public:
    S_ResourceData(const S_String &name, const size_t &size, const unsigned char *data );

    S_String name() const;
    size_t size() const;
    const unsigned char *data() const;

private:
    S_String m_name;
    size_t   m_size;
    const unsigned char *m_data;
};

class S_ResourcesContainer
{
public:
    S_ResourcesContainer();
    const S_UnorderedMap<S_String, std::unique_ptr<S_ResourceData>>* resources() const;

private:
    void updateResources(size_t numberOfResources, const unsigned char *fileNames, const size_t *fileSizes, const unsigned char *filesData );
    S_UnorderedMap<S_String, std::unique_ptr<S_ResourceData>> m_resources;

};

}

#include "S_Resources.inl"
