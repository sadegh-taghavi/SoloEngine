#include <stdint.h>
#include "core/algorithm/S_String.h"

class S_Debug
{
    friend class OutputGenerator;
    void outDebug();
    S_String m_tag;
    S_String m_data;
public:
    S_Debug();
    S_Debug &debug( const S_Debug &data, const S_String &tag = "Solo_Engine");
    S_Debug &operator<<( const S_String &val );
    S_Debug &operator<<( const uint64_t &val );
    S_Debug &operator<<( const int64_t &val );
    S_Debug &operator<<( const float &val );
    S_Debug &operator<<( const double &val );
    S_Debug &operator<<( const unsigned int &val );
    S_Debug &operator<<( const int &val );
};
