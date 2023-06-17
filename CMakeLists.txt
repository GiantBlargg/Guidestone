cmake_minimum_required(VERSION 3.26)

project(Guidestone CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CXXFLAGS -Wall -Wextra -pedantic)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/)

find_package(SDL2 2.26 REQUIRED CONFIG)
find_package(Vulkan 1.3 REQUIRED)
target_compile_definitions(Vulkan::Headers INTERFACE VK_NO_PROTOTYPES VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0 VULKAN_HPP_NO_SMART_HANDLE)

target_link_libraries(${PROJECT_NAME} SDL2::SDL2 Vulkan::Headers)
