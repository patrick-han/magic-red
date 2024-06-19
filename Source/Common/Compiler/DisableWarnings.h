#pragma once
#include <Common/Platform.h>
#if PLATFORM_WINDOWS
#define PUSH_MSVC_WARNINGS __pragma(warning( push ))
#define DISABLE_MSVC_WARNING( warningNumber ) __pragma(warning( disable : warningNumber))
#define POP_MSVC_WARNINGS __pragma(warning( pop ))
#else
#define PUSH_MSVC_WARNINGS
#define DISABLE_MSVC_WARNING( warningNumber )
#define POP_MSVC_WARNINGS
#endif

#if PLATFORM_MACOS
#define PUSH_CLANG_WARNINGS _Pragma("clang diagnostic push")
#define POP_CLANG_WARNINGS _Pragma("clang diagnostic pop")
#define CLANG_PRAGMA(x) _Pragma(#x)
#define DISABLE_CLANG_WARNING(x) CLANG_PRAGMA(clang diagnostic ignored x)
#else
#define PUSH_CLANG_WARNINGS
#define DISABLE_CLANG_WARNING(x)
#define POP_CLANG_WARNINGS
#endif