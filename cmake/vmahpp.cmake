include(FetchContent)
FetchContent_Declare(vmahpp
	GIT_REPOSITORY https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git
	GIT_TAG eca325b
	GIT_SUBMODULES VulkanMemoryAllocator
)

FetchContent_GetProperties(vmahpp)
if(NOT vmahpp_POPULATED)
	FetchContent_Populate(vmahpp)
endif()

add_library(VulkanMemoryAllocator-Hpp INTERFACE)
target_include_directories(VulkanMemoryAllocator-Hpp SYSTEM INTERFACE ${vmahpp_SOURCE_DIR}/include ${vmahpp_SOURCE_DIR}/VulkanMemoryAllocator/include)
