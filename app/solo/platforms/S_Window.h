#pragma once

#include "S_Event.h"
#include "S_InputEvent.h"
#include <memory>

namespace solo
{

class S_EventFIFO
{
  public:
    S_EventFIFO();
    ~S_EventFIFO();
    bool isEmpty();
    void push(std::unique_ptr<S_Event> item);
    std::unique_ptr<S_Event> pop();

private:
    static const unsigned char m_BUFFER_SIZE = 8;
    int m_head, m_tail;
    std::unique_ptr<S_Event> m_buffer[m_BUFFER_SIZE] = {};
};

class S_Window
{

public:
    S_Window();
    virtual ~S_Window();

    bool focus() const;
    void close();
    bool textInput() const;
    void setTextInput(bool textInput);
    virtual void setSize(unsigned int width, unsigned int height) = 0;
    virtual std::unique_ptr<S_Event> getEvent(bool wait_for_event = true) = 0;

    bool running() const;

    unsigned int width() const;

    unsigned int height() const;

protected:
    static const unsigned char m_MAX_KEY_STATE = 255;
    S_TouchEvent m_touchEvent;
    bool         m_keyState[m_MAX_KEY_STATE];
    S_MouseEvent m_mouseEvent;
    S_EventFIFO  m_eventFIFO;
    unsigned int m_width;
    unsigned int m_height;
    bool         m_focus;
    bool         m_running;
    bool         m_textInput;
};

}
