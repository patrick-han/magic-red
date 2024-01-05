set(CMAKE_CXX_STANDARD 20)
cmake_minimum_required(VERSION 3.20)

add_library(Dependencies INTERFACE)

# ========== KHRONOS UTILITIES ========== #
# set(SPIRV_WERROR OFF)
# add_subdirectory(glslang)
# add_subdirectory(SPIRV-Headers)
# add_subdirectory(SPIRV-Tools)

# add_library(SPIRV-Reflect STATIC SPIRV-Reflect/spirv_reflect.h SPIRV-Reflect/spirv_reflect.c)
# target_link_libraries(Dependencies INTERFACE SPIRV-Reflect)
# target_include_directories(Dependencies INTERFACE SPIRV-Reflect/)

# ========== VulkanMemoryAllocator ========== #
set(VMA_STATIC_VULKAN_FUNCTIONS OFF)
add_subdirectory(VulkanMemoryAllocator)
target_include_directories(Dependencies SYSTEM INTERFACE VulkanMemoryAllocator/include/ -w)

# ========== SHADERC ========== #
# set(SHADERC_SKIP_TESTS ON)
# set(SHADERC_SKIP_EXAMPLES ON)
# set(SHADERC_ENABLE_TESTS OFF)

# add_subdirectory(shaderc)
# target_link_libraries(Dependencies INTERFACE shaderc)

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

# ========== IMGUI ========== #
add_library(IMGUI STATIC)
target_sources(IMGUI PRIVATE 
    imgui/imgui.h
    imgui/imgui.cpp 
    

    imgui/imgui_demo.cpp 
    imgui/imgui_draw.cpp 
    imgui/imgui_widgets.cpp 
    imgui/imgui_tables.cpp 
    imgui/imgui_impl_vulkan.cpp 
    imgui/imgui_impl_glfw.cpp 
    )
target_include_directories(IMGUI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/glfw/include/" "${Vulkan_INCLUDE_DIR}")
target_link_libraries(Dependencies INTERFACE IMGUI)
target_include_directories(Dependencies INTERFACE imgui)
    

# ========== ASSIMP ========== #
# set(ASSIMP_NO_EXPORT ON)
# set(ASSIMP_BUILD_TESTS OFF)
# set(ASSIMP_INSTALL OFF)
# set(ASSIMP_WARNINGS_AS_ERRORS OFF)
# if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
#     set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)
# endif()

# add_subdirectory(Assimp)
# target_link_libraries(Dependencies INTERFACE assimp)
# target_include_directories(Dependencies SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Assimp/include/)