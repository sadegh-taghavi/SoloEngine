#pragma once
#include "S_Window.h"
#include "S_Event.h"
#include "S_InputEvent.h"
#include "S_WindowEvent.h"
#include "solo/stl/S_Array.h"
#include <memory>

namespace solo
{

class S_InputState
{
public:
    S_InputState();
    bool isKey(S_Key key) const;
    bool isKey(S_MouseButton mouseButton) const;
    unsigned int mouseX() const;
    unsigned int mouseY() const;
    float mouseDeltaX() const;
    float mouseDeltaY() const;
    int mouseZ() const;
    void updateState( const S_MouseEvent *event );
    void updateState( const S_KeyboardEvent *event );
    void update();

private:
    S_Array<bool, static_cast<uint32_t>(S_Key::Count) > m_keysState;
    S_Array<bool, static_cast<uint32_t>(S_MouseButton::Count) > m_mouseState;
    unsigned int    m_mX;
    unsigned int    m_mY;
    unsigned int    m_mLastX;
    unsigned int    m_mLastY;
    int             m_mZ;
    float           m_mDeltaX;
    float           m_mDeltaY;
};

class S_BaseApplication
{
    friend class S_Application;
public:
    virtual void onMouseEvent(const S_MouseEvent *event);
    virtual void onTouchEvent(const S_TouchEvent *event);
    virtual void onKeyboardEvent(const S_KeyboardEvent *event);
    virtual void onResizeEvent(const S_WindowResizeEvent *event);
    virtual void onFocusEvent(const S_WindowFocusEvent *event);
    virtual void onCreateEvent() = 0;
    virtual void onPaintEvent();
    virtual void onCloseEvent();
    virtual void onSpinEvent();
    bool exec( bool wait = true );
    void eventLoop();
    S_Window *window() const;
    static S_BaseApplication *executingApplication();
    const S_InputState *inputState() const;

private:
    S_BaseApplication( unsigned int width, unsigned int height );
    virtual ~S_BaseApplication();
    static S_BaseApplication *m_activeApplication;
    std::unique_ptr<S_Window> m_window;
    S_InputState m_inputState;
};

}
