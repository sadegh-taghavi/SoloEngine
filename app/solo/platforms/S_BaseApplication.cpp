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
#include "solo/ui/S_ImGuiLayer.h"
#include "solo/ui/S_UI.h"
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
    m_activeApplication = this;
}

S_BaseApplication::~S_BaseApplication()
{

}

const S_InputState *S_BaseApplication::inputState() const
{
    return &m_inputState;
}

S_InputMap *S_BaseApplication::inputMap()
{
    return &m_inputMap;
}

const S_GestureRecognizer *S_BaseApplication::gestures() const
{
    return &m_gestures;
}

const S_GamepadState *S_BaseApplication::gamepad() const
{
    return &m_gamepad.state();
}

void S_BaseApplication::onMouseEvent(const S_MouseEvent *event )
{
    // UI-first routing: ImGui, then native UI, then gameplay. Release events
    // always reach gameplay so held buttons can't get stuck behind a capture.
    bool captured = false;
    if( auto* imgui = S_ImGuiLayer::instance() )
    {
        imgui->onMouseEvent( event );
        captured = imgui->wantCaptureMouse();
    }
    if( auto* ui = S_UI::instance() )
    {
        if( !captured )
            ui->onMouseEvent( event );
        captured = captured || ui->wantCaptureMouse();
    }

    if( !captured || event->state() == S_MouseEventState::Up )
        m_inputState.updateState( event );

    // mouse doubles as a single gesture pointer (id -1) for desktop testing
    if( !captured && event->button() == S_MouseButton::Left )
    {
        const float x = static_cast<float>( event->x() ), y = static_cast<float>( event->y() );
        if( event->state() == S_MouseEventState::Down ) m_gestures.pointerDown( -1, x, y );
        if( event->state() == S_MouseEventState::Move ) m_gestures.pointerMove( -1, x, y );
    }
    if( event->state() == S_MouseEventState::Up )
        m_gestures.pointerUp( -1, static_cast<float>( event->x() ), static_cast<float>( event->y() ) );

    s_debugLayer( "onMouseEvent", event->x(), event->y(), event->z(),
                  static_cast<uint32_t>( event->button() ), static_cast<uint32_t>( event->state() ) );
}

void S_BaseApplication::onTouchEvent(const S_TouchEvent *event)
{
    if( auto* ui = S_UI::instance() )
        ui->onTouchEvent( event );

    auto* self = const_cast<S_TouchEvent*>( event );
    bool uiCaptured = S_UI::instance() && S_UI::instance()->wantCaptureMouse();
    for( unsigned int i = 0; i < 10; ++i )
    {
        const S_TouchPoint& p = self->pointByIndex( i );
        if( p.id() == 0 && p.x() == 0.0f && p.y() == 0.0f ) continue; // unused slot
        switch( p.state() )
        {
        case S_TouchEventState::Down: if( !uiCaptured ) m_gestures.pointerDown( p.id(), p.x(), p.y() ); break;
        case S_TouchEventState::Move: m_gestures.pointerMove( p.id(), p.x(), p.y() ); break;
        case S_TouchEventState::Up:   m_gestures.pointerUp( p.id(), p.x(), p.y() );   break;
        }
    }
    s_debugLayer( "onTouchEvent", event->activeCount() );
}

void S_BaseApplication::onKeyboardEvent(const S_KeyboardEvent *event)
{
    bool captured = false;
    if( auto* imgui = S_ImGuiLayer::instance() )
    {
        imgui->onKeyboardEvent( event );
        captured = imgui->wantCaptureKeyboard();
    }
    if( !captured || event->state() == S_KeyboardEventState::Up )
        m_inputState.updateState( event );
    s_debugLayer( "onKeyboardEvent", static_cast<uint32_t>( event->key() ), static_cast<uint32_t>( event->state() ) );
}

void S_BaseApplication::onCharacterEvent(const S_CharacterEvent *event)
{
    if( auto* imgui = S_ImGuiLayer::instance() )
        imgui->onCharacterEvent( event );
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
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>( now - m_lastSpin ).count();
    m_lastSpin = now;

    m_gestures.update( dt );
    m_gamepad.poll();
    m_inputMap.update( m_inputState, m_gamepad.state() );
    m_inputState.update();
}

bool S_BaseApplication::exec(bool wait)
{
    m_activeApplication = this;
    std::unique_ptr<S_Event> e;
    while ( ( e = m_window->getEvent( wait ) ).get() && e->type() != S_EventType::None )
    {
        // type tags are set by event constructors, so static_cast is safe — no RTTI
        switch (e->type())
        {
        case S_EventType::Mouse:
            onMouseEvent( static_cast<S_MouseEvent*>(e.get()) );
            break;
        case S_EventType::Touch:
            onTouchEvent( static_cast<S_TouchEvent*>(e.get()) );
            break;
        case S_EventType::Keyboard:
            onKeyboardEvent( static_cast<S_KeyboardEvent*>(e.get()) );
            break;
        case S_EventType::Character:
            onCharacterEvent( static_cast<S_CharacterEvent*>(e.get()) );
            break;
        case S_EventType::WindowCreate:
            onCreateEvent();
            break;
        case S_EventType::WindowResize:
            onResizeEvent( static_cast<S_WindowResizeEvent*>(e.get()) );
            break;
        case S_EventType::WindowFocus:
            onFocusEvent( static_cast<S_WindowFocusEvent*>(e.get()) );
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
    if( event->state() == S_MouseEventState::Wheel )
    {
        m_mZ += event->z(); // accumulated until end-of-frame update()
        return;
    }
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
