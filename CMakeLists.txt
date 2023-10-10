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
configure_file(Include/Helpers/RootDir.h.in ${CMAKE_BINARY_DIR}/Include/Helpers/RootDir.h)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/Include)
include_directories(${CMAKE_BINARY_DIR}/Include/Helpers)
include_directories(${CMAKE_BINARY_DIR}/Include/External)

# Add SOURCE_FILES and HEADER_FILES variables, will traverse all subdirectories
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_SOURCE_DIR}/Source/*.cpp)
# file(GLOB_RECURSE HEADER_FILES ${CMAKE_SOURCE_DIR}/include/*.h)

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Dependencies
find_package(Vulkan REQUIRED)
add_subdirectory(Dependencies)

if (VULKAN_FOUND)
    message(STATUS "Found Vulkan, Including and Linking now")
    include_directories(${Vulkan_INCLUDE_DIRS})
    target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} Dependencies)
endif (VULKAN_FOUND)