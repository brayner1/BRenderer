# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("BRenderer")

add_subdirectory ("BRenderer/dependencies/spdlog")
add_subdirectory ("BRenderer/dependencies/assimp")
add_subdirectory ("BRenderer/dependencies/VulkanMemoryAllocator")
add_subdirectory ("BRenderer/dependencies/rigtorp/SPSCQueue")
add_subdirectory ("BRenderer/dependencies/rigtorp/MPMCQueue")

# Include sub-projects.
add_subdirectory ("BRenderer")