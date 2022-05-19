#pragma once
#include "S_Resources.h"
#include "solo/debug/S_Debug.h"

using namespace solo;

inline const S_UnorderedMap<S_String, std::unique_ptr<S_ResourceData> > *S_ResourcesContainer::resources() const
{
    return &m_resources;
}

inline void S_ResourcesContainer::updateResources(size_t numberOfResources, const unsigned char *fileNames, const size_t *fileSizes, const unsigned char *filesData)
{
    size_t fileNamesIndex;
    size_t fileDataIndex = 0;
    S_String filename;
    for( size_t i = 0; i < numberOfResources; ++i )
    {
        fileNamesIndex = 64 * i;
        filename = reinterpret_cast<const char *>( &fileNames[ fileNamesIndex ] );
        m_resources[ filename ] = std::make_unique<S_ResourceData>( filename, fileSizes[ i ], &filesData[ fileDataIndex ] );
        fileDataIndex += fileSizes[ i ];
    }
}

inline S_ResourceData::S_ResourceData(const S_String &name, const size_t &size, const unsigned char *data) :
    m_name(name), m_size(size), m_data(data)

{

}

inline S_String S_ResourceData::name() const
{
    return m_name;
}

inline size_t S_ResourceData::size() const
{
    return m_size;
}

inline const unsigned char *S_ResourceData::data() const
{
    return m_data;
}

