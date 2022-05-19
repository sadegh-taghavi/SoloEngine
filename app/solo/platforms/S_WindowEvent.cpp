#include "S_WindowEvent.h"
#include <cstring>

using namespace solo;

S_WindowEvent::S_WindowEvent(const S_EventType &type) : S_Event( type )
{

}

S_WindowCreateEvent::S_WindowCreateEvent() : S_WindowEvent ( S_EventType::WindowCreate )
{

}

S_WindowResizeEvent::S_WindowResizeEvent(unsigned int width, unsigned int height) :
    S_WindowEvent ( S_EventType::WindowResize ), m_width(width), m_height(height)
{

}

unsigned int S_WindowResizeEvent::width() const
{
    return m_width;
}

void S_WindowResizeEvent::setWidth(unsigned int width)
{
    m_width = width;
}

unsigned int S_WindowResizeEvent::height() const
{
    return m_height;
}

void S_WindowResizeEvent::setHeight(unsigned int height)
{
    m_height = height;
}


S_WindowPaintEvent::S_WindowPaintEvent() : S_WindowEvent ( S_EventType::WindowPaint )
{

}

S_WindowCloseEvent::S_WindowCloseEvent() : S_WindowEvent ( S_EventType::WindowClose )
{

}

S_WindowFocusEvent::S_WindowFocusEvent(bool focus) : S_WindowEvent ( S_EventType::WindowFocus ), m_focus(focus)
{

}

bool S_WindowFocusEvent::focus() const
{
    return m_focus;
}

void S_WindowFocusEvent::setFocus(bool focus)
{
    m_focus = focus;
}
