cmake_minimum_required(VERSION 3.26)

project(Guidestone CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

# Core Engine

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/core/*.cpp)
add_library(${PROJECT_NAME}Core ${SOURCES})
target_include_directories(${PROJECT_NAME}Core PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/core/)

#Vulkan Renderer

include(FetchContent)
FetchContent_Declare(Vulkan-Headers
	GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
	GIT_TAG main
)
FetchContent_MakeAvailable(Vulkan-Headers)
target_compile_definitions(Vulkan-Headers INTERFACE VK_NO_PROTOTYPES VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0 VULKAN_HPP_NO_SMART_HANDLE)

include(vmahpp)
target_compile_definitions(VulkanMemoryAllocator-Hpp INTERFACE VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=0)

include(shaders)
set(shader_root ${CMAKE_CURRENT_LIST_DIR}/src/render/vulkan/shaders)
file(GLOB_RECURSE shader_sources RELATIVE ${shader_root} CONFIGURE_DEPENDS ${shader_root}/*.vert ${shader_root}/*.frag)
make_shader_lib(shaders "Vulkan::Shaders" vulkan1.3 ${shader_root} ${shader_sources})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/render/vulkan/*.cpp)
add_library(${PROJECT_NAME}Vulkan ${SOURCES})
target_include_directories(${PROJECT_NAME}Vulkan PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/render/vulkan)
target_link_libraries(${PROJECT_NAME}Vulkan ${PROJECT_NAME}Core Vulkan::Headers VulkanMemoryAllocator-Hpp shaders)


#SDL Platform

find_package(SDL2 2.26 REQUIRED CONFIG)

add_executable(${PROJECT_NAME}SDL src/platform/SDL/SDL_platform.cpp)
target_link_libraries(${PROJECT_NAME}SDL ${PROJECT_NAME}Core ${PROJECT_NAME}Vulkan SDL2::SDL2)
