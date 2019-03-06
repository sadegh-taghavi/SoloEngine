#include "S_Debug.h"
#ifdef __ANDROID__
#include <android/log.h>
#elif __WIN32__
#include <stdio.h>
#endif

using namespace solo;

S_Debug::S_Debug()
{
    m_tag = "SoloEngine";
    m_data = "";
}

void S_Debug::outDebug()
{
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, m_tag.c_str(), "%s", m_data.c_str() );
#elif __WIN32__
    printf( "%s:%s", m_tag.c_str(), m_data.c_str() );
    fflush(stdout);
#endif
}

S_Debug &S_Debug::debug(const S_Debug &data, const S_String &tag)
{
    m_data = data.m_data;
    m_tag = tag;
    m_data += "\n";
    outDebug();
    return *this;
}

S_Debug &S_Debug::operator<<(const S_String &val)
{
    m_data += " " + val;
    return *this;
}

S_Debug &S_Debug::operator<<(const uint64_t &val)
{
    char str[25];
    sprintf( str, " %llu", val );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const int64_t &val)
{
    char str[25];
    sprintf( str, " %lld", val );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const unsigned int &val)
{
    char str[25];
    sprintf( str, " %d", val );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const int &val)
{
    char str[25];
    sprintf( str, " %d", val );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const float &val)
{
    char str[25];
    sprintf( str, " %f", static_cast<double>(val) );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const double &val)
{
    char str[25];
    sprintf( str, " %lf", val );
    m_data += str;
    return *this;
}

//S_Debug &S_Debug::operator<<(const bool &val)
//{
//    m_data += ( val ? " TRUE" : " FALSE" );
//    return *this;
//}

