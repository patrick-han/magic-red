set(CMAKE_CXX_STANDARD 20)
cmake_minimum_required(VERSION 3.20)

add_library(Dependencies INTERFACE)

# ========== KHRONOS UTILITIES ========== #
set(SPIRV_WERROR OFF)
add_subdirectory(glslang)
add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)

add_library(SPIRV-Reflect STATIC SPIRV-Reflect/spirv_reflect.h SPIRV-Reflect/spirv_reflect.c)
target_link_libraries(Dependencies INTERFACE SPIRV-Reflect)
target_include_directories(Dependencies INTERFACE SPIRV-Reflect/)

# ========== SHADERC ========== #
set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)
set(SHADERC_ENABLE_TESTS OFF)

add_subdirectory(shaderc)
target_link_libraries(Dependencies INTERFACE shaderc)

# ========== GLFW ========== #
set(GLFW_BUILD_EXAMPLES, OFF)
set(GLFW_BUILD_TESTS, OFF)
set(GLFW_BUILD_DOCS, OFF)

add_subdirectory(GLFW)
target_link_libraries(Dependencies INTERFACE glfw)
target_include_directories(Dependencies INTERFACE GLFW/include/)

# ========== GLM ========== #
add_subdirectory(glm)
target_link_libraries(Dependencies INTERFACE glm)
target_include_directories(Dependencies INTERFACE glm)