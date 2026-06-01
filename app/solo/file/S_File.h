#pragma once
#include <string>
#include <fstream>
#include <iostream>

namespace solo
{

enum class S_FileOpenMode
{
    Read = std::ios::in,
    Write = std::ios::out,
    Append = std::ios::app,
    Truncate = std::ios::trunc,
};

class S_ResourceData;

class S_File
{
public:
    S_File( const std::string &fileName );
    virtual ~S_File();
    std::string fileName() const;
    void setFileName(const std::string &fileName);
    bool isOpen() const;
    bool exist() const;
    size_t size() const;
    bool open( S_FileOpenMode mode = S_FileOpenMode::Read );
    void close();
    bool read( char *buffer, uint64_t length );
    bool write( const char *buffer, uint64_t length );
    bool isResourceFile() const;
    S_FileOpenMode openMode() const;
   const S_ResourceData *resourceData() const;

private:
    bool m_isResourceFile;
    std::string m_fileName;
    bool m_isOpen;
    std::fstream m_fileStream;
    const S_ResourceData *m_resourceData;
    S_FileOpenMode m_openMode;

};

}

