#pragma once

#include "solo/stl/S_String.h"
#include "solo/thread/S_Mutex.h"
#include <stdint.h>

namespace solo
{

#define s_debug(...) ( S_Debug().debug( ( S_Debug(),__VA_ARGS__ ) ) )

#ifdef SOLO_ENABLE_DEBUG_LAYER
#define s_debugLayer(...) ( S_Debug().debug( ( S_Debug(),__VA_ARGS__ ), "Solo_Engine_Debug_Layer" ) )
#else
#define s_debugLayer(...) (#__VA_ARGS__)
#endif


class S_Debug
{
    friend class OutputGenerator;
    void outDebug();
    S_String m_tag;
    S_String m_data;
    S_AtomicFlag m_busyState;
public:
    S_Debug();
    S_Debug &debug( const S_Debug &data, const S_String &tag = "Solo_Engine");
    S_Debug &operator<<( const S_String &val );
    S_Debug &operator,( const S_String &val ) { return this->operator <<(val); }
    S_Debug &operator<<( const uint64_t &val );
    S_Debug &operator,( const uint64_t &val ) { return this->operator <<(val); }
    S_Debug &operator<<( const int64_t &val );
    S_Debug &operator,( const int64_t &val ) { return this->operator <<(val); }
    S_Debug &operator<<( const float &val );
    S_Debug &operator,( const float &val ) { return this->operator <<(val); }
    S_Debug &operator<<( const double &val );
    S_Debug &operator,( const double &val ) { return this->operator <<(val); }
    S_Debug &operator<<( const unsigned int &val );
    S_Debug &operator,( const unsigned int &val ) { return this->operator <<(val); }
    S_Debug &operator<<( const int &val );
    S_Debug &operator,( const int &val ) { return this->operator <<(val); }
//    S_Debug &operator<<( const bool &val );
//    S_Debug &operator,( const bool &val ) { return this->operator <<(val); }
};

}
