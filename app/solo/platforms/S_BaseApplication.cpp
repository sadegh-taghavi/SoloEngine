#include "solo/platforms/S_SystemDetect.h"

#include "S_BaseApplication.h"
#ifdef S_PLATFORM_WINDOWS
#include "S_WindowWin32.h"
#elif defined(S_PLATFORM_LINUX)
#include "S_WindowX11.h"
#elif defined(S_PLATFORM_ANDROID)
#include "S_WindowAndroid.h"
#endif

#include "solo/debug/S_Debug.h"
#include <stdint.h>

using namespace solo;

S_BaseApplication *S_BaseApplication::m_activeApplication = nullptr;

S_BaseApplication::S_BaseApplication(unsigned int width, unsigned int height)
{
#ifdef S_PLATFORM_WINDOWS
    m_window = std::make_unique<S_WindowWin32>( width, height );
#elif defined(S_PLATFORM_LINUX)
    m_window = std::make_unique<S_WindowX11>( width, height );
#elif defined(S_PLATFORM_ANDROID)
    m_window = std::make_unique<S_WindowAndroid>( width, height );
#endif
    //m_activeApplication = this;
}

S_BaseApplication::~S_BaseApplication()
{

}

const S_InputState *S_BaseApplication::inputState() const
{
    return &m_inputState;
}

void S_BaseApplication::onMouseEvent(const S_MouseEvent *event )
{
    m_inputState.updateState( event );
    s_debugLayer( "onMouseEvent", event->x(), event->y(), event->z(),
                  static_cast<uint32_t>( event->button() ), static_cast<uint32_t>( event->state() ) );
}

void S_BaseApplication::onTouchEvent(const S_TouchEvent *event)
{
    s_debugLayer( "onTouchEvent", event->activeCount() );
}

void S_BaseApplication::onKeyboardEvent(const S_KeyboardEvent *event)
{
    m_inputState.updateState( event );
    s_debugLayer( "onKeyboardEvent", static_cast<uint32_t>( event->key() ), static_cast<uint32_t>( event->state() ) );
}

void S_BaseApplication::onResizeEvent(const S_WindowResizeEvent *event)
{
    s_debugLayer( "onResizeEvent", event->width(), event->height() );
}

void S_BaseApplication::onFocusEvent(const S_WindowFocusEvent *event)
{
    s_debugLayer( "onFocusEvent", event->focus() );
}

void S_BaseApplication::onCreateEvent()
{

}

void S_BaseApplication::onPaintEvent()
{
    s_debugLayer("onPaintEvent");
}

void S_BaseApplication::onCloseEvent()
{
    s_debugLayer("onCloseEvent");
}

void S_BaseApplication::onSpinEvent()
{
    m_inputState.update();
}

bool S_BaseApplication::exec(bool wait)
{
    m_activeApplication = this;
    std::unique_ptr<S_Event> e;
    while ( ( e = m_window->getEvent( wait ) ).get() && e->type() != S_EventType::None )
    {
        switch (e->type())
        {
        case S_EventType::Mouse:
            onMouseEvent( dynamic_cast<S_MouseEvent*>(e.get()) );
            break;
        case S_EventType::Touch:
            onTouchEvent( dynamic_cast<S_TouchEvent*>(e.get()) );
            break;
        case S_EventType::Keyboard:
            onKeyboardEvent( dynamic_cast<S_KeyboardEvent*>(e.get()) );
            break;
        case S_EventType::WindowCreate:
            onCreateEvent();
            break;
        case S_EventType::WindowResize:
            onResizeEvent( dynamic_cast<S_WindowResizeEvent*>(e.get()) );
            break;
        case S_EventType::WindowFocus:
            onFocusEvent ( dynamic_cast<S_WindowFocusEvent*>(e.get()) );
            break;
        case S_EventType::WindowPaint:
            onPaintEvent();
            break;
        case S_EventType::WindowClose:
            onCloseEvent();
            break;
        default:
            onSpinEvent();
            break;
        }
    }
    return m_window->running();
}

void S_BaseApplication::eventLoop()
{
    m_window->getEvent( false );
}

S_Window* S_BaseApplication::window() const
{
    return m_window.get();
}

S_BaseApplication *S_BaseApplication::executingApplication()
{
    return m_activeApplication;
}

unsigned int S_InputState::mouseX() const
{
    return m_mX;
}

unsigned int S_InputState::mouseY() const
{
    return m_mY;
}

float S_InputState::mouseDeltaX() const
{
    return m_mDeltaX;
}

float S_InputState::mouseDeltaY() const
{
    return m_mDeltaY;
}

int S_InputState::mouseZ() const
{
    return m_mZ;
}

void S_InputState::updateState(const S_MouseEvent *event)
{
    m_mX = event->x();
    m_mY = event->y();
    m_mZ = event->z();
    if( event->state() != S_MouseEventState::Move )
    {
        m_mouseState[ static_cast<uint32_t>( event->button() ) ] = event->state() == S_MouseEventState::Down;
        m_mLastX = m_mX;
        m_mLastY = m_mY;
    }
}

void S_InputState::updateState(const S_KeyboardEvent *event)
{
    m_keysState[ static_cast<uint32_t>( event->key() ) ] = event->state() == S_KeyboardEventState::Down;
}

void S_InputState::update()
{
    m_mDeltaX = static_cast<float>(m_mX) - static_cast<float>(m_mLastX);
    m_mDeltaY = static_cast<float>(m_mY) - static_cast<float>(m_mLastY);
    m_mLastX = m_mX;
    m_mLastY = m_mY;
    m_mZ = 0;
}

S_InputState::S_InputState()
{
    m_mLastX = 0;
    m_mLastY = 0;
    m_mDeltaX = 0;
    m_mDeltaY = 0;
    m_mX = 0;
    m_mY = 0;
    m_mZ = 0;
    std::fill( m_keysState.begin(), m_keysState.end(), false );
    std::fill( m_mouseState.begin(), m_mouseState.end(), false );
}

bool S_InputState::isKey(S_Key key) const
{
    return m_keysState[static_cast<uint32_t>( key )];
}

bool S_InputState::isKey(S_MouseButton mouseButton) const
{
    return m_mouseState[static_cast<uint32_t>( mouseButton )];
}
