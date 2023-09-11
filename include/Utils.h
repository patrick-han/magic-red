#pragma once
#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

// Logging

#define MRLOG(msg) \
    std::cout << "[MRLOG] " << msg << std::endl

#define MRWARN(msg) \
    std::cout << "[WARN] " << msg << std::endl

#define DEBUG(msg) \
    std::cout << "[DEBUG] " << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl

#define MRCERR(msg) \
    std::cerr << "[CERR] " << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl


// Debug and platform build defines

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

#ifdef __APPLE__
    const bool appleBuild = true;
#else
    const bool appleBuild = false;
#endif
