#include "S_InputEvent.h"
#include <cstring>

using namespace solo;

S_KeyboardEvent::S_KeyboardEvent(const S_Key &key, const S_KeyboardEventState &state) :
    S_Event ( S_EventType::Keyboard ), m_key(key), m_state(state)
{

}

S_Key S_KeyboardEvent::key() const
{
    return m_key;
}

void S_KeyboardEvent::setKey(const S_Key &key)
{
    m_key = key;
}

S_KeyboardEventState S_KeyboardEvent::state() const
{
    return m_state;
}

void S_KeyboardEvent::setState(const S_KeyboardEventState &state)
{
    m_state = state;
}

S_MouseEvent::S_MouseEvent(const S_MouseButton &button, const S_MouseEventState &state,
                           unsigned int x, unsigned int y, int z)
: S_Event ( S_EventType::Mouse ), m_button(button), m_state(state), m_x(x), m_y(y), m_z(z)
{

}

S_MouseButton S_MouseEvent::button() const
{
    return m_button;
}

void S_MouseEvent::setButton(const S_MouseButton &button)
{
    m_button = button;
}

S_MouseEventState S_MouseEvent::state() const
{
    return m_state;
}

void S_MouseEvent::setState(const S_MouseEventState &state)
{
    m_state = state;
}

unsigned int S_MouseEvent::x() const
{
    return m_x;
}

void S_MouseEvent::setX(unsigned int x)
{
    m_x = x;
}

unsigned int S_MouseEvent::y() const
{
    return m_y;
}

void S_MouseEvent::setY(unsigned int y)
{
    m_y = y;
}

int S_MouseEvent::z() const
{
    return m_z;
}

void S_MouseEvent::setZ(int z)
{
    m_z = z;
}

S_CharacterEvent::S_CharacterEvent(const char *character) : S_Event ( S_EventType::Character )
{
    setCharacter(character);
}

const char *S_CharacterEvent::character() const
{
    return m_character;
}

void S_CharacterEvent::setCharacter(const char *character)
{
    memcpy(m_character, character, 4);
}

S_TouchPoint::S_TouchPoint(const S_TouchEventState &state, float x, float y, int id) :
    m_state( state ), m_x( x ), m_y( y ), m_id(id)
{

}

float S_TouchPoint::x() const
{
    return m_x;
}

void S_TouchPoint::setX(float x)
{
    m_x = x;
}

float S_TouchPoint::y() const
{
    return m_y;
}

void S_TouchPoint::setY(float y)
{
    m_y = y;
}

int S_TouchPoint::id() const
{
    return m_id;
}

void S_TouchPoint::setId(int id)
{
    m_id = id;
}

S_TouchEventState S_TouchPoint::state() const
{
    return m_state;
}

void S_TouchPoint::setState(const S_TouchEventState &state)
{
    m_state = state;
}

void S_TouchPoint::operator=(const S_TouchPoint &other)
{
    m_id = other.m_id;
    m_state = other.m_state;
    m_x = other.m_x;
    m_y = other.m_y;
}

S_TouchEvent::S_TouchEvent(): S_Event( S_EventType::Touch ), m_activeCount( 0 )
{
}

unsigned int S_TouchEvent::activeCount() const
{
    return m_activeCount;
}

void S_TouchEvent::setActiveCount(unsigned int activeCount)
{
    m_activeCount = activeCount;
}

const S_TouchPoint &S_TouchEvent::pointByIndex(unsigned int index)
{
    if( index < m_MAX_TOUCH_POINTS )
        return m_touchPoints[ index ];
    return m_touchPoints[0];
}

const S_TouchPoint &S_TouchEvent::pointByID(int id)
{
    for( unsigned int i = 0; i < m_MAX_TOUCH_POINTS; ++i )
    {
        if( m_touchPoints[i].id() == id )
            return m_touchPoints[i];
    }
    return m_touchPoints[0];
}

void S_TouchEvent::setPointByIndex(unsigned int index, const S_TouchPoint &point)
{
    if( index < m_MAX_TOUCH_POINTS )
        m_touchPoints[index] = point;

}

void S_TouchEvent::setPointByID(int id, const S_TouchPoint &point)
{
    for( unsigned int i = 0; i < m_MAX_TOUCH_POINTS; ++i )
    {
        if( m_touchPoints[i].id() == id )
        {
            m_touchPoints[i] = point;
            break;
        }
    }
}

void S_TouchEvent::operator=(S_TouchEvent &other)
{
    for( unsigned int i = 0; i < m_MAX_TOUCH_POINTS; ++i )
        m_touchPoints[i] = other.m_touchPoints[i];
    m_activeCount = other.m_activeCount;
}
