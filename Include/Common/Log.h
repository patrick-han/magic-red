#pragma once

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