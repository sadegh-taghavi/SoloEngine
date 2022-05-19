#include "solo/platforms/S_SystemDetect.h"
#ifdef S_PLATFORM_WINDOWS
#include "S_WindowWin32.h"
#include "S_WindowEvent.h"
#include "S_InputEvent.h"
#include "solo/debug/S_Debug.h"
#include <memory>

using namespace solo;


int main(int, char **)
{
    return soloMain();
}

#define WM_RESHAPE (WM_USER + 0)
#define WM_ACTIVE  (WM_USER + 1)

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        PostMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        return 0;
    case WM_GETMINMAXINFO:
        return 0;
    case WM_SIZE:
    case WM_EXITSIZEMOVE:
        PostMessage(hWnd, WM_RESHAPE, 0, 0);
        break;
    case WM_ACTIVATE:
        PostMessage(hWnd, WM_ACTIVE, wParam, lParam);
        break;
    default:
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

S_WindowWin32::S_WindowWin32(unsigned int width, unsigned int height)
{
    m_width  = width;
    m_height = height;
    m_running = true;

    m_hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX win_class;
    win_class.cbSize        = sizeof(WNDCLASSEX);
    win_class.style         = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc   = WndProc;
    win_class.cbClsExtra    = 0;
    win_class.cbWndExtra    = 0;
    win_class.hInstance     = m_hInstance;
    win_class.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    win_class.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    win_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    win_class.lpszMenuName  = nullptr;
    win_class.lpszClassName = L"Solo";
    win_class.hInstance     = m_hInstance;
    win_class.hIconSm       = LoadIcon(nullptr, IDI_WINLOGO);

    RegisterClassEx(&win_class);

    RECT wr = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    m_hWnd = CreateWindowEx(0,
                            L"Solo",
                            L"Solo",
                            WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            wr.right - wr.left,
                            wr.bottom - wr.top,
                            nullptr,
                            nullptr,
                            m_hInstance,
                            nullptr);
    m_eventFIFO.push(std::make_unique<S_WindowCreateEvent>());
    m_eventFIFO.push(std::make_unique<S_WindowResizeEvent>(width, height));
}

S_WindowWin32::~S_WindowWin32()
{
    DestroyWindow(m_hWnd);
}

void S_WindowWin32::setSize(unsigned int width, unsigned height)
{
    RECT wr = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    int total_width = wr.right - wr.left;
    int total_height = wr.bottom - wr.top;
    SetWindowPos(m_hWnd, nullptr, 0, 0, total_width, total_height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
    if ((width != m_width) | (height != m_height))
        m_eventFIFO.push(std::make_unique<S_WindowResizeEvent>(width, height));
}

std::unique_ptr<S_Event> solo::S_WindowWin32::getEvent(bool wait_for_event)
{
    if (!m_eventFIFO.isEmpty())
        return m_eventFIFO.pop();

    MSG msg = {};
    if (wait_for_event)
        m_running = (GetMessage(&msg, nullptr, 16, 0) > 0);
    else
        m_running = (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0);

    std::unique_ptr<S_Event> event;
    if (m_running)
    {
        TranslateMessage(&msg);
        int x = GET_X_LPARAM(msg.lParam);
        int y = GET_Y_LPARAM(msg.lParam);

        if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP)
        {
            if (msg.wParam == VK_CONTROL)
                msg.wParam = (msg.lParam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
            if (msg.wParam == VK_SHIFT)
            {
                if (!!(GetKeyState(VK_LSHIFT) & 128) != m_keyState[static_cast<unsigned int>(S_Key::LeftShift)])
                    PostMessage(m_hWnd, msg.message, VK_LSHIFT, 0);
                if (!!(GetKeyState(VK_RSHIFT) & 128) != m_keyState[static_cast<unsigned int>(S_Key::LeftShift)])
                    PostMessage(m_hWnd, msg.message, VK_RSHIFT, 0);
            }
        }else if (msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP)
        {
            if (msg.wParam == VK_MENU)
                msg.wParam = (msg.lParam & (1 << 24)) ? VK_RMENU : VK_LMENU;
        }

        switch (msg.message)
        {
//        case WM_PAINT:
//        {
//            event = std::make_unique<S_WindowPaintEvent>();
//            break;
//        }
        case WM_MOUSEMOVE  :
        {
            m_mouseEvent.setState( S_MouseEventState::Move );
            m_mouseEvent.setX( static_cast<unsigned int>(x) );
            m_mouseEvent.setY( static_cast<unsigned int>(y) );
            event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
            break;
        }
        case WM_LBUTTONDOWN:
        {
            m_mouseEvent.setState( S_MouseEventState::Down );
            m_mouseEvent.setButton( S_MouseButton::Left );
            m_mouseEvent.setX( static_cast<unsigned int>(x) );
            m_mouseEvent.setY( static_cast<unsigned int>(y) );
            event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
            break;
        }
        case WM_MBUTTONDOWN:
        {
            m_mouseEvent.setState( S_MouseEventState::Down );
            m_mouseEvent.setButton( S_MouseButton::Middle );
            m_mouseEvent.setX( static_cast<unsigned int>(x) );
            m_mouseEvent.setY( static_cast<unsigned int>(y) );
            event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
            break;
        }
        case WM_RBUTTONDOWN:
        {
            m_mouseEvent.setState( S_MouseEventState::Down );
            m_mouseEvent.setButton( S_MouseButton::Right );
            m_mouseEvent.setX( static_cast<unsigned int>(x) );
            m_mouseEvent.setY( static_cast<unsigned int>(y) );
            event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
            break;
        }
        case WM_LBUTTONUP  :
        {
            m_mouseEvent.setState( S_MouseEventState::Up );
            m_mouseEvent.setButton( S_MouseButton::Left );
            m_mouseEvent.setX( static_cast<unsigned int>(x) );
            m_mouseEvent.setY( static_cast<unsigned int>(y) );
            event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
            break;
        }
        case WM_MBUTTONUP  :
        {
            m_mouseEvent.setState( S_MouseEventState::Up );
            m_mouseEvent.setButton( S_MouseButton::Middle );
            m_mouseEvent.setX( static_cast<unsigned int>(x) );
            m_mouseEvent.setY( static_cast<unsigned int>(y) );
            event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
            break;
        }
        case WM_RBUTTONUP  :
        {
            m_mouseEvent.setState( S_MouseEventState::Up );
            m_mouseEvent.setButton( S_MouseButton::Right );
            m_mouseEvent.setX( static_cast<unsigned int>(x) );
            m_mouseEvent.setY( static_cast<unsigned int>(y) );
            event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
            break;
        }
        case WM_MOUSEWHEEL:
        {
            int wheel = /*(*/GET_WHEEL_DELTA_WPARAM(msg.wParam) /*> 0) ? 4 : 5*/;
            POINT point = {x, y};
            ScreenToClient(msg.hwnd, &point);
            m_mouseEvent.setX( static_cast<unsigned int>(point.x) );
            m_mouseEvent.setY( static_cast<unsigned int>(point.y) );
            m_mouseEvent.setZ( wheel );
            event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
            break;
        }
        case WM_KEYDOWN   :
            event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( WIN32_TO_HID[msg.wParam]), S_KeyboardEventState::Down );
            break;
        case WM_KEYUP     :
            event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( WIN32_TO_HID[msg.wParam]), S_KeyboardEventState::Up );
            break;
        case WM_SYSKEYDOWN:
        {
            MSG discard;
            GetMessage(&discard, nullptr, 0, 0);
            event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( WIN32_TO_HID[msg.wParam]), S_KeyboardEventState::Down );
            break;
        }
        case WM_SYSKEYUP  :
            event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( WIN32_TO_HID[msg.wParam]), S_KeyboardEventState::Up );
            break;
        case WM_CHAR:
        {
            char buff[4];
            strncpy_s(buff, reinterpret_cast<const char *>( &msg.wParam ), 4);
            event = std::make_unique<S_CharacterEvent>(buff);
            break;
        }
        case WM_ACTIVE:
        {
            m_focus = msg.wParam != WA_INACTIVE;
            event = std::make_unique<S_WindowFocusEvent>(m_focus);
            break;
        }
        case WM_RESHAPE:
        {
//            if (!m_focus)
//            {
//                m_focus = false;
//                PostMessage(m_hWnd, WM_RESHAPE, msg.wParam, msg.lParam);
//                event = std::make_unique<S_WindowFocusEvent>(m_focus);
//            }

            RECT r;
            GetClientRect(m_hWnd, &r);
            unsigned int w = static_cast<unsigned int>(r.right - r.left);
            unsigned int h = static_cast<unsigned int>(r.bottom - r.top);
            std::unique_ptr<S_Event> ev;
            if (w != m_width || h != m_height)
            {
                m_width = w;
                m_height = h;
                ev = std::make_unique<S_WindowResizeEvent>(w, h);
            }
            else
                ev = std::make_unique<S_Event>(S_EventType::Other);
            if( event.get() )
                m_eventFIFO.push( std::move(ev) );
            else
                event = std::move(ev);
            break;
        }
        case WM_CLOSE:
        {
            event = std::make_unique<S_WindowCloseEvent>();
            m_eventFIFO.push( std::make_unique<S_Event>() );
            break;
            /*#ifdef ENABLE_MULTITOUCH
            case WM_POINTERUPDATE:
            case WM_POINTERDOWN:
            case WM_POINTERUP: {
                POINTER_INFO pointerInfo;
                if (GetPointerInfo(GET_POINTERID_WPARAM(msg.wParam), &pointerInfo))
                {
                    uint  id = pointerInfo.pointerId;
                    POINT pt = pointerInfo.ptPixelLocation;
                    ScreenToClient(hWnd, &pt);
                    switch (msg.message) {
                    case WM_POINTERDOWN  : return ;  // touch down event
                    case WM_POINTERUPDATE: return ;  // touch move event
                    case WM_POINTERUP    : return ;  // touch up event
                    }
                }
            }
    #endif*/
        }
        default: event = std::make_unique<S_Event>(S_EventType::Other);
        }
        DispatchMessage(&msg);
    }
    if( event.get() == nullptr )
        event = std::make_unique<S_Event>(S_EventType::Other);
    return event;
}

HINSTANCE S_WindowWin32::hInstance() const
{
    return m_hInstance;
}

HWND S_WindowWin32::hWnd() const
{
    return m_hWnd;
}

#endif

