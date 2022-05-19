#pragma once
#include "solo/platforms/S_SystemDetect.h"
#ifdef S_PLATFORM_LINUX

#include "S_Window.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>


extern int soloMain();

namespace solo
{

const unsigned char X11_TO_HID[256] = {
      0,  0,  0,  0,  0,  0,  0,  0, 42, 43,  0,  0,  0, 40,  0,  0,
    225,224,226, 72, 57,  0,  0,  0,  0,  0,  0, 41,  0,  0,  0,  0,
     44, 75, 78, 77, 74, 80, 82, 79, 81,  0,  0,  0, 70, 73, 76,  0,
     39, 30, 31, 32, 33, 34, 35, 36, 37, 38,  0,  0,  0,  0,  0,  0,
      0,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
     19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,  0,  0,  0,  0,  0,
     98, 89, 90, 91, 92, 93, 94, 95, 96, 97, 85, 87,  0, 86, 99, 84,
     58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,104,105,106,107,
    108,109,110,111,112,113,114,115,  0,  0,  0,  0,  0,  0,  0,  0,
     83, 71,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    225,229,224,228,226,230,  0,  0,  0,  0,  0,  0,  0,127,128,129,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 51, 46, 54, 45, 55, 56,
     53,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 47, 49, 48, 52,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

class S_WindowX11 : public S_Window {

  public:
    S_WindowX11(unsigned int width,  unsigned int height);
    virtual ~S_WindowX11();
    std::unique_ptr<S_Event> getEvent(bool wait_for_event = true);
    void setSize( unsigned int width,  unsigned int height ); 
    Display *display() const;
    Window window() const;
    GC gc() const;
private:

    Display *m_display;
		int m_screen;
		Window m_window;
		GC m_gc;
};
}
#endif
