#pragma once
#include <cstdlib>
#include <Common/Platform.h>

#if PLATFORM_WINDOWS
inline constexpr uint32_t WINDOW_WIDTH = 2500;
inline constexpr uint32_t WINDOW_HEIGHT = 1200;
#else
inline constexpr uint32_t WINDOW_WIDTH = 1500;
inline constexpr uint32_t WINDOW_HEIGHT = 800;
#endif
inline constexpr int MAX_FRAMES_IN_FLIGHT = 2;