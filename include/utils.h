#pragma once

#include <iostream>

#define MRLOG(msg) \
    std::cout << "[MRLOG] " << msg << std::endl

#define DEBUG(msg) \
    std::cout << "[DEBUG] " << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl