cmake_minimum_required(VERSION 3.20)

project(magic-red VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

ADD_CUSTOM_TARGET(debug
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
  COMMENT "Creating the executable in the debug mode.")

ADD_CUSTOM_TARGET(release
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
  COMMENT "Creating the executable in the release mode.")

# Configure assets header file to help us find resources
configure_file(Source/Common/Generators/RootDir.h.in ${CMAKE_SOURCE_DIR}/Source/Common/RootDir.h)


# Configure platform file
if(MSVC)
  set(PLATFORM_WINDOWS 1)
  set(PLATFORM_MACOS 0)
elseif(APPLE)
  set(PLATFORM_WINDOWS 0)
  set(PLATFORM_MACOS 1)
endif ()
configure_file(Source/Common/Generators/Platform.h.in ${CMAKE_SOURCE_DIR}/Source/Common/Platform.h)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/Source)
# include_directories(${CMAKE_BINARY_DIR}/Source/Common)
include_directories(${CMAKE_BINARY_DIR}/Source/External)

# Add SOURCE_FILES and HEADER_FILES variables, will traverse all subdirectories
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_SOURCE_DIR}/Source/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_SOURCE_DIR}/Source/*.h)

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# Warnings as errors
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()


# Dependencies
find_package(Vulkan REQUIRED)
add_subdirectory(Dependencies)

if (VULKAN_FOUND)
    message(STATUS "Found Vulkan, Including and Linking now")
    include_directories(${Vulkan_INCLUDE_DIRS})
    target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} Dependencies)
endif (VULKAN_FOUND)



# Shader compile target
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/Shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/Shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/Shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "Building shaders")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/Shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(shaders
    DEPENDS ${SPIRV_BINARY_FILES})