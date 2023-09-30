# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.14)
enable_testing()

project ("BRenderer")

add_subdirectory ("BRenderer/dependencies/spdlog")
add_subdirectory ("BRenderer/dependencies/VulkanMemoryAllocator")

# Include sub-projects.
add_subdirectory ("BRenderer")