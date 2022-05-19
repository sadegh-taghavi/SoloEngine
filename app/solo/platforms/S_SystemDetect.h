#pragma once

#ifdef __WINNT__
#define S_PLATFORM_WINDOWS 1
#endif

#if defined( __ANDROID__ )
#define S_PLATFORM_ANDROID 1
#elif defined(__linux__)
#define S_PLATFORM_LINUX 1
#endif

#ifdef __MINGW64__
#define S_COMPILER_MINGW64 1
#endif

#ifdef __MINGW32__
#define S_COMPILER_MINGW32 1
#endif

#ifdef __clang__
#define S_COMPILER_CLANG 1
#endif
