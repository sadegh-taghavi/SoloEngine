#include "solo/platforms/S_SystemDetect.h"
#ifdef S_PLATFORM_LINUX
#include "S_WindowX11.h"
#include "S_WindowEvent.h"
#include "S_InputEvent.h"
#include "solo/debug/S_Debug.h"
#include <memory>

using namespace solo;

int main(int, char **)
{
    return soloMain();
}

Display* S_WindowX11::display() const
{
    return m_display;
}

Window S_WindowX11::window() const
{
    return m_window;
}

GC S_WindowX11::gc() const
{
    return m_gc;
}

S_WindowX11::S_WindowX11(unsigned int width, unsigned int height)
{
    m_width  = width;
    m_height = height;
    m_running = true;


	m_display = XOpenDisplay(nullptr);
   	m_screen = DefaultScreen(m_display);
	unsigned long black = BlackPixel(m_display, m_screen);
	unsigned long white = WhitePixel(m_display, m_screen);

   	m_window = XCreateSimpleWindow( m_display, DefaultRootWindow(m_display), 0, 0, m_width, m_height, 2, white, black);


	XSetStandardProperties( m_display, m_window, "Solo", "Solo", None, nullptr, 0, nullptr);

	XSelectInput(m_display, m_window, EnterWindowMask | LeaveWindowMask | ExposureMask | FocusChangeMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

    m_gc = XCreateGC(m_display, m_window, 0, 0);        

	XSetBackground(m_display, m_gc, white);
	XSetForeground(m_display, m_gc, black);

	XClearWindow(m_display, m_window);
	XMapRaised(m_display, m_window);

    m_eventFIFO.push(std::make_unique<S_WindowCreateEvent>());
    m_eventFIFO.push(std::make_unique<S_WindowResizeEvent>(width, height));
}

S_WindowX11::~S_WindowX11()
{
    XFreeGC(m_display, m_gc);
	XDestroyWindow(m_display, m_window);
	XCloseDisplay(m_display);	
}

void S_WindowX11::setSize(unsigned int width, unsigned height)
{
    XResizeWindow(m_display, m_window, m_width, m_height);

    if ((width != m_width) | (height != m_height))
        m_eventFIFO.push(std::make_unique<S_WindowResizeEvent>(width, height));
}

std::unique_ptr<S_Event> solo::S_WindowX11::getEvent(bool wait_for_event)
{
    if (!m_eventFIFO.isEmpty())
        return m_eventFIFO.pop();

    XEvent xEvent;
    if( wait_for_event )
        XNextEvent(m_display, &xEvent);
    else
        XCheckWindowEvent( m_display, m_window, EnterWindowMask | LeaveWindowMask | ExposureMask | FocusChangeMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask, &xEvent );
    std::unique_ptr<S_Event> event;
    if (m_running)
    {
        switch (xEvent.type)
        {
            case Expose:
            {
                event = std::make_unique<S_WindowPaintEvent>();
                break;
            }
            case FocusIn:
            {
                m_focus = true;
                event = std::make_unique<S_WindowFocusEvent>(m_focus);
                break;
            }
            case FocusOut:
            {
                m_focus = false;
                event = std::make_unique<S_WindowFocusEvent>(m_focus);
                break;
            }
            case KeyPress:
            {
                char buffer[100];
                int n = XLookupString(&xEvent.xkey, buffer, 1, nullptr, nullptr);
                buffer[n] = '\0';
                
                event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( buffer[0] ), S_KeyboardEventState::Down );
                break;
            }
            case KeyRelease:
            {
                 char buffer[100];
                int n = XLookupString(&xEvent.xkey, buffer, 1, nullptr, nullptr);
                buffer[n] = '\0';
                event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( buffer[0] ), S_KeyboardEventState::Up );
                break;
            }
            case ConfigureNotify:
            {
                auto xConfigureEvent = xEvent.xconfigure;
                if ((xConfigureEvent.width != m_width) | (xConfigureEvent.height != m_height))
                {
                    m_width = xConfigureEvent.width;
                    m_height = xConfigureEvent.height;
                    event = std::make_unique<S_WindowResizeEvent>(m_width, m_height);
                }
                break;
            }
            case DestroyNotify:
            {
                
                event = std::make_unique<S_WindowCloseEvent>();
                break;
            }
            
            case MotionNotify:
            case LeaveNotify:
            case EnterNotify:
            {
                auto xCrossing = xEvent.xcrossing;
                
                m_mouseEvent.setState( S_MouseEventState::Move );
                m_mouseEvent.setX( static_cast<unsigned int>(xCrossing.x) );
                m_mouseEvent.setY( static_cast<unsigned int>(xCrossing.y) );

                event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
                break;
            }
            case ButtonPress:
            {
                auto xbutton = xEvent.xbutton;
                m_mouseEvent.setState( S_MouseEventState::Down );
                m_mouseEvent.setX( static_cast<unsigned int>(xbutton.x) );
                m_mouseEvent.setY( static_cast<unsigned int>(xbutton.y) );

                switch (xbutton.button)
                {
                case Button1:
                {
                    m_mouseEvent.setButton( S_MouseButton::Left );
                    break; 
                }
                case Button2:
                {
                    m_mouseEvent.setButton( S_MouseButton::Middle );
                    break;  
                }
                case Button3: 
                {
                    m_mouseEvent.setButton( S_MouseButton::Right );    
                    break;
                }
                case Button4:
                {
                    m_mouseEvent.setZ( 1 );
                    break;
                }
                case Button5:
                {
                    m_mouseEvent.setZ( -1 );
                    break;
                }
                }
                
                event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
                break;
            }
            case ButtonRelease:
            {
                auto xbutton = xEvent.xbutton;
                m_mouseEvent.setState( S_MouseEventState::Up );
                m_mouseEvent.setX( static_cast<unsigned int>(xbutton.x) );
                m_mouseEvent.setY( static_cast<unsigned int>(xbutton.y) );

                switch (xbutton.button)
                {
                case Button1:
                {
                    m_mouseEvent.setButton( S_MouseButton::Left );
                    break; 
                }
                case Button2:
                {
                    m_mouseEvent.setButton( S_MouseButton::Middle );
                    break;  
                }
                case Button3: 
                {
                    m_mouseEvent.setButton( S_MouseButton::Right );    
                    break;
                }
                case Button4:
                {
                    m_mouseEvent.setZ( 0 );
                    break;
                }
                case Button5:
                {
                    m_mouseEvent.setZ( 0 );
                    break;
                }
                }
                
                event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
                break;
            }
            
            default: event = std::make_unique<S_Event>(S_EventType::Other);

        
		// if (xEvent.type==KeyPress&&
		//     XLookupString(&xEvent.xkey,text,255,&key,0)==1) {
		// /* use the XLookupString routine to convert the invent
		//    KeyPress data into regular text.  Weird but necessary...
		// */
		// 	if (text[0]=='q') {
		// 		close_x();
		// 	}
		// 	printf("You pressed the %c key!\n",text[0]);
		// }
		// if (xEvent.type==ButtonPress) {
		// /* tell where the mouse Button was Pressed */
		// 	printf("You pressed a button at (%i,%i)\n",
		// 		xEvent.xbutton.x,xEvent.xbutton.y);
		// }
//         TranslateMessage(&msg);
//         int x = GET_X_LPARAM(msg.lParam);
//         int y = GET_Y_LPARAM(msg.lParam);

//         if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP)
//         {
//             if (msg.wParam == VK_CONTROL)
//                 msg.wParam = (msg.lParam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
//             if (msg.wParam == VK_SHIFT)
//             {
//                 if (!!(GetKeyState(VK_LSHIFT) & 128) != m_keyState[static_cast<unsigned int>(S_Key::LeftShift)])
//                     PostMessage(m_hWnd, msg.message, VK_LSHIFT, 0);
//                 if (!!(GetKeyState(VK_RSHIFT) & 128) != m_keyState[static_cast<unsigned int>(S_Key::LeftShift)])
//                     PostMessage(m_hWnd, msg.message, VK_RSHIFT, 0);
//             }
//         }else if (msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP)
//         {
//             if (msg.wParam == VK_MENU)
//                 msg.wParam = (msg.lParam & (1 << 24)) ? VK_RMENU : VK_LMENU;
//         }

//         switch (msg.message)
//         {
// //        case WM_PAINT:
// //        {
// //            event = std::make_unique<S_WindowPaintEvent>();
// //            break;
// //        }
//         case WM_MOUSEMOVE  :
//         {
//             m_mouseEvent.setState( S_MouseEventState::Move );
//             m_mouseEvent.setX( static_cast<unsigned int>(x) );
//             m_mouseEvent.setY( static_cast<unsigned int>(y) );
//             event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
//             break;
//         }
//         case WM_LBUTTONDOWN:
//         {
//             m_mouseEvent.setState( S_MouseEventState::Down );
//             m_mouseEvent.setButton( S_MouseButton::Left );
//             m_mouseEvent.setX( static_cast<unsigned int>(x) );
//             m_mouseEvent.setY( static_cast<unsigned int>(y) );
//             event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
//             break;
//         }
//         case WM_MBUTTONDOWN:
//         {
//             m_mouseEvent.setState( S_MouseEventState::Down );
//             m_mouseEvent.setButton( S_MouseButton::Middle );
//             m_mouseEvent.setX( static_cast<unsigned int>(x) );
//             m_mouseEvent.setY( static_cast<unsigned int>(y) );
//             event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
//             break;
//         }
//         case WM_RBUTTONDOWN:
//         {
//             m_mouseEvent.setState( S_MouseEventState::Down );
//             m_mouseEvent.setButton( S_MouseButton::Right );
//             m_mouseEvent.setX( static_cast<unsigned int>(x) );
//             m_mouseEvent.setY( static_cast<unsigned int>(y) );
//             event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
//             break;
//         }
//         case WM_LBUTTONUP  :
//         {
//             m_mouseEvent.setState( S_MouseEventState::Up );
//             m_mouseEvent.setButton( S_MouseButton::Left );
//             m_mouseEvent.setX( static_cast<unsigned int>(x) );
//             m_mouseEvent.setY( static_cast<unsigned int>(y) );
//             event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
//             break;
//         }
//         case WM_MBUTTONUP  :
//         {
//             m_mouseEvent.setState( S_MouseEventState::Up );
//             m_mouseEvent.setButton( S_MouseButton::Middle );
//             m_mouseEvent.setX( static_cast<unsigned int>(x) );
//             m_mouseEvent.setY( static_cast<unsigned int>(y) );
//             event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
//             break;
//         }
//         case WM_RBUTTONUP  :
//         {
//             m_mouseEvent.setState( S_MouseEventState::Up );
//             m_mouseEvent.setButton( S_MouseButton::Right );
//             m_mouseEvent.setX( static_cast<unsigned int>(x) );
//             m_mouseEvent.setY( static_cast<unsigned int>(y) );
//             event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
//             break;
//         }
//         case WM_MOUSEWHEEL:
//         {
//             int wheel = /*(*/GET_WHEEL_DELTA_WPARAM(msg.wParam) /*> 0) ? 4 : 5*/;
//             POINT point = {x, y};
//             ScreenToClient(msg.hwnd, &point);
//             m_mouseEvent.setX( static_cast<unsigned int>(point.x) );
//             m_mouseEvent.setY( static_cast<unsigned int>(point.y) );
//             m_mouseEvent.setZ( wheel );
//             event = std::make_unique<S_MouseEvent>( m_mouseEvent.button(), m_mouseEvent.state(), m_mouseEvent.x(), m_mouseEvent.y(), m_mouseEvent.z() );
//             break;
//         }
//         case WM_KEYDOWN   :
//             event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( WIN32_TO_HID[msg.wParam]), S_KeyboardEventState::Down );
//             break;
//         case WM_KEYUP     :
//             event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( WIN32_TO_HID[msg.wParam]), S_KeyboardEventState::Up );
//             break;
//         case WM_SYSKEYDOWN:
//         {
//             MSG discard;
//             GetMessage(&discard, nullptr, 0, 0);
//             event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( WIN32_TO_HID[msg.wParam]), S_KeyboardEventState::Down );
//             break;
//         }
//         case WM_SYSKEYUP  :
//             event = std::make_unique<S_KeyboardEvent>( static_cast<S_Key>( WIN32_TO_HID[msg.wParam]), S_KeyboardEventState::Up );
//             break;
//         case WM_CHAR:
//         {
//             char buff[4];
//             strncpy_s(buff, reinterpret_cast<const char *>( &msg.wParam ), 4);
//             event = std::make_unique<S_CharacterEvent>(buff);
//             break;
//         }
//         case WM_ACTIVE:
//         {
//             m_focus = msg.wParam != WA_INACTIVE;
//             event = std::make_unique<S_WindowFocusEvent>(m_focus);
//             break;
//         }
//         case WM_RESHAPE:
//         {
// //            if (!m_focus)
// //            {
// //                m_focus = false;
// //                PostMessage(m_hWnd, WM_RESHAPE, msg.wParam, msg.lParam);
// //                event = std::make_unique<S_WindowFocusEvent>(m_focus);
// //            }

//             RECT r;
//             GetClientRect(m_hWnd, &r);
//             unsigned int w = static_cast<unsigned int>(r.right - r.left);
//             unsigned int h = static_cast<unsigned int>(r.bottom - r.top);
//             std::unique_ptr<S_Event> ev;
//             if (w != m_width || h != m_height)
//             {
//                 m_width = w;
//                 m_height = h;
//                 ev = std::make_unique<S_WindowResizeEvent>(w, h);
//             }
//             else
//                 ev = std::make_unique<S_Event>(S_EventType::Other);
//             if( event.get() )
//                 m_eventFIFO.push( std::move(ev) );
//             else
//                 event = std::move(ev);
//             break;
//         }
//         case WM_CLOSE:
//         {
//             event = std::make_unique<S_WindowCloseEvent>();
//             m_eventFIFO.push( std::make_unique<S_Event>() );
//             break;
//             /*#ifdef ENABLE_MULTITOUCH
//             case WM_POINTERUPDATE:
//             case WM_POINTERDOWN:
//             case WM_POINTERUP: {
//                 POINTER_INFO pointerInfo;
//                 if (GetPointerInfo(GET_POINTERID_WPARAM(msg.wParam), &pointerInfo))
//                 {
//                     uint  id = pointerInfo.pointerId;
//                     POINT pt = pointerInfo.ptPixelLocation;
//                     ScreenToClient(hWnd, &pt);
//                     switch (msg.message) {
//                     case WM_POINTERDOWN  : return ;  // touch down event
//                     case WM_POINTERUPDATE: return ;  // touch move event
//                     case WM_POINTERUP    : return ;  // touch up event
//                     }
//                 }
//             }
//     #endif*/
//         }
//         default: event = std::make_unique<S_Event>(S_EventType::Other);
//         }
//         DispatchMessage(&msg);
        }
    }
    if( event.get() == nullptr )
        event = std::make_unique<S_Event>(S_EventType::Other);
    return event;
}

#endif

