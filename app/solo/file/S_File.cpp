#include "S_File.h"
#include "solo/application/S_Application.h"
#include "solo/resource/S_ResourceManager.h"
#include "solo/resource/S_Resources.h"
#include "solo/debug/S_Debug.h"

using namespace solo;

S_File::S_File( const S_String &fileName ) : m_isOpen( false ), m_resourceData( nullptr ), m_openMode( S_FileOpenMode::Read )
{
    setFileName( fileName );
}

S_File::~S_File()
{
    close();
}

S_String S_File::fileName() const
{
    return m_fileName;
}

void S_File::setFileName(const S_String &fileName)
{
    m_fileName = fileName;
    if( fileName.length() > 4 && fileName.substr( 0 , 4 ) == "sr:/" )
        m_isResourceFile = true;
}

bool S_File::exist() const
{
    if( m_isResourceFile )
        return ( S_Application::executingApplication()->
                 resourceManager()->resourceData( m_fileName ) != nullptr );
    std::ifstream file( m_fileName, std::ios_base::in );
    if( file.is_open() )
    {
        file.close();
        return true;
    }
    return false;
}

size_t S_File::size() const
{
    if( m_isResourceFile )
    {
        auto fd = S_Application::executingApplication()->
                resourceManager()->resourceData( m_fileName );
        if( fd )
            return fd->size();
        return 0;
    }
    std::ifstream file( m_fileName, std::ios_base::in );
    if( file.is_open() )
    {
        auto begin = file.tellg();
        file.seekg (0, std::ios::end);
        auto end = file.tellg();
        size_t fileSize = static_cast<size_t>(end - begin);
        file.close();
        return fileSize;
    }
    return 0;
}

bool S_File::isOpen() const
{
    return m_isOpen;
}

bool S_File::open(S_FileOpenMode mode)
{
    if( m_isOpen )
    {
        if( mode == m_openMode  )
            return true;
        else
            close();
    }
    if( m_isResourceFile )
        m_isOpen = ( ( m_resourceData = S_Application::executingApplication()->
                     resourceManager()->resourceData( m_fileName ) ) != nullptr );
    else
    {
        m_fileStream.open( m_fileName, static_cast<std::ios_base::openmode>( m_openMode ) );
        m_isOpen = m_fileStream.is_open();
    }
    return  m_isOpen;
}

void S_File::close()
{
    m_resourceData = nullptr;
    m_isOpen = false;
    if( m_fileStream.is_open() )
        m_fileStream.close();
}

bool S_File::read(char *buffer, uint64_t length)
{
    if( !m_isOpen )
        return false;
    if( m_isResourceFile )
    {
        if( length > m_resourceData->size() )
            return false;
        memcpy( buffer, m_resourceData->data(), length );
        return true;
    }
    m_fileStream.read( buffer, static_cast<std::streamsize>( length ) );
    return m_fileStream.good();
}

bool S_File::write(const char *buffer, uint64_t length)
{
    if( !m_isOpen )
        return false;
    if( m_isResourceFile )
        return false;
    m_fileStream.write( buffer, static_cast<std::streamsize>( length ) );
    return !m_fileStream.bad();
}

bool S_File::isResourceFile() const
{
    return m_isResourceFile;
}

S_FileOpenMode S_File::openMode() const
{
    return m_openMode;
}

const S_ResourceData *S_File::resourceData() const
{
    return m_resourceData;
}
