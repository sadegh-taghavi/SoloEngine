#include "S_Window.h"
#include "solo/memory/S_Allocator.h"
#include <cstring>

using namespace solo;

S_EventFIFO::S_EventFIFO() : m_head(0), m_tail(0) {}

S_EventFIFO::~S_EventFIFO()
{

}

bool S_EventFIFO::isEmpty()
{
    return m_head == m_tail;
}

void S_EventFIFO::push(std::unique_ptr<S_Event> item)
{
    ++m_head;
    m_buffer[m_head %= m_BUFFER_SIZE] = std::move(item);
}

std::unique_ptr<S_Event> S_EventFIFO::pop()
{
    if (m_head == m_tail)
        return std::unique_ptr<S_Event>();
    ++m_tail;
    return std::move( m_buffer[ m_tail %= m_BUFFER_SIZE ] );
}

S_Window::S_Window() : m_width(0), m_height(0), m_focus(false),
    m_running(false), m_textInput(false)
{
    memset(m_keyState, 0, sizeof(m_keyState));
}

S_Window::~S_Window()
{

}

void S_Window::close()
{
    m_running = false;
}

bool S_Window::textInput() const
{
    return m_textInput;
}

void S_Window::setTextInput(bool textInput)
{
    m_textInput = textInput;
}

bool S_Window::running() const
{
    return m_running;
}

unsigned int S_Window::width() const
{
    return m_width;
}

unsigned int S_Window::height() const
{
    return m_height;
}
