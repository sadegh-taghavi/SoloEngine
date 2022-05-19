#include "S_Debug.h"
#include "solo/platforms/S_SystemDetect.h"
#ifdef S_PLATFORM_ANDROID
#include <android/log.h>
#elif defined( S_PLATFORM_WINDOWS )
#include <stdio.h>
#elif defined( S_PLATFORM_LINUX )
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
    S_AtomicFlagLocker locker( &m_busyState );
#ifdef S_PLATFORM_ANDROID
    __android_log_print(ANDROID_LOG_DEBUG, m_tag.c_str(), "%s", m_data.c_str() );
#elif defined( S_PLATFORM_WINDOWS )
    printf( "%s:%s", m_tag.c_str(), m_data.c_str() );
    fflush(stdout);
#elif defined( S_PLATFORM_LINUX )
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
    char str[64];
    sprintf( str, " %llu", val );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const int64_t &val)
{
    char str[64];
    sprintf( str, " %lld", val );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const unsigned int &val)
{
    char str[64];
    sprintf( str, " %d", val );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const int &val)
{
    char str[64];
    sprintf( str, " %d", val );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const float &val)
{
    char str[64];
    sprintf( str, " %f", static_cast<double>(val) );
    m_data += str;
    return *this;
}

S_Debug &S_Debug::operator<<(const double &val)
{
    char str[64];
    sprintf( str, " %lf", val );
    m_data += str;
    return *this;
}

//S_Debug &S_Debug::operator<<(bool &val)
//{
//    m_data += ( val ? " TRUE" : " FALSE" );
//    return *this;
//}

