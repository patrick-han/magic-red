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