#include "core/algorithm/S_String.h"

class S_Debug
{
    void out(const S_String &str );
    S_String m_tag;
public:
    S_Debug();
    void setTag(const S_String &tag);
    S_String &tag();
    S_Debug &operator<<( const S_String &val );
};
