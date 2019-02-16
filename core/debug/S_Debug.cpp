#include "S_Debug.h"
#ifdef __ANDROID__
#include <android/log.h>
#elif __WIN32__
#include <stdio.h>
#endif

S_Debug::S_Debug()
{
    m_tag = "SoloEngine";
}

void S_Debug::out(const S_String &str)
{
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, m_tag.c_str(), str.c_str() );
#elif __WIN32__
    printf( "%s:%s", m_tag.c_str(), str.c_str() );
#endif
}

void S_Debug::setTag(const S_String &tag)
{
    m_tag = tag;
}

S_String &S_Debug::tag()
{
    return m_tag;
}

S_Debug &S_Debug::operator<<(const S_String &val)
{
    out( " " + val );
    return *this;
}

