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

# ========== SDL ========== #
set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
add_subdirectory(SDL)
target_link_libraries(Dependencies INTERFACE SDL3::SDL3)

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
    imgui/imgui_impl_sdl3.cpp 
    )
target_include_directories(IMGUI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/SDL/include/" "${Vulkan_INCLUDE_DIR}")
target_link_libraries(Dependencies INTERFACE IMGUI)
target_include_directories(Dependencies INTERFACE imgui)
    

# ========== NVRHI ========== #
set(NVRHI_BUILD_SHARED OFF CACHE BOOL "" FORCE)

set(NVRHI_WITH_DX11 OFF CACHE BOOL "" FORCE)
set(NVRHI_WITH_DX12 OFF CACHE BOOL "" FORCE)
set(NVRHI_WITH_VULKAN ON CACHE BOOL "" FORCE)

add_subdirectory(nvrhi)
target_link_libraries(Dependencies INTERFACE nvrhi)
target_link_libraries(Dependencies INTERFACE nvrhi_vk)