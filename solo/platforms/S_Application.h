#pragma once

#ifdef __WINNT__
#include "win32/S_WindowManager.h"
#elif defined ( __ANDROID__ )
#include "android/S_WindowManager.h"
#endif
namespace solo
{

//#define main soloMain

extern int soloMain(int, char **);

class S_Application
{

};

}
