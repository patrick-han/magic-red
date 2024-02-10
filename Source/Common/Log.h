#pragma once

#include <iostream>

#define MRLOG(msg) \
    std::cout << "[LOG] " << msg << std::endl

#define MRWARN(msg) \
    std::cout << "[WARNING] " << msg << std::endl

#define DEBUG(msg) \
    std::cout << "[DEBUG] " << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl

#define MRCERR(msg) \
    std::cerr << "[CERR] " << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl

#define MRVAL(msg) \
    std::cerr << "[VULKAN] " << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl