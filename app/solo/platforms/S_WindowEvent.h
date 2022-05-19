#pragma once
#include "S_Event.h"

namespace solo
{

class S_WindowEvent : public S_Event
{
public:
    S_WindowEvent(const S_EventType &type);
};

class S_WindowCreateEvent : public S_WindowEvent
{
public:
    S_WindowCreateEvent();

private:

};

class S_WindowResizeEvent : public S_WindowEvent
{
public:
    S_WindowResizeEvent(unsigned int width, unsigned int height);

    unsigned int width() const;
    void setWidth(unsigned int width);

    unsigned int height() const;
    void setHeight(unsigned int height);

private:
    unsigned int m_width;
    unsigned int m_height;


};

class S_WindowPaintEvent : public S_WindowEvent
{
public:
    S_WindowPaintEvent();

private:

};

class S_WindowCloseEvent : public S_WindowEvent
{
public:
    S_WindowCloseEvent();

private:

};

class S_WindowFocusEvent : public S_WindowEvent
{
public:
    S_WindowFocusEvent(bool focus);

    bool focus() const;
    void setFocus(bool focus);

private:
    bool m_focus;

};



}
