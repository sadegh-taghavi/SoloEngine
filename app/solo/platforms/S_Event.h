#pragma once

namespace solo
{

enum class S_EventType
{
    None           = 0,
    WindowCreate   = 1,
    WindowResize   = 2,
    WindowPaint    = 4,
    WindowClose    = 8,
    WindowFocus    = 16,
    Keyboard       = 32,
    Character      = 64,
    Mouse          = 128,
    Touch          = 256,
    Other          = 512,
};

class S_Event
{
public:
    S_Event( const S_EventType &type = S_EventType::None );
    virtual ~S_Event();
    S_EventType type() const;
    void setType(const S_EventType &type);

protected:
    S_EventType m_type;
};

}
