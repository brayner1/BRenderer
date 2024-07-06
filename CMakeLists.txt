# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("BRenderer")

set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_BUILD_TESTS OFF)

add_subdirectory ("3rdparties/spdlog")
add_subdirectory ("3rdparties/assimp")
add_subdirectory ("3rdparties/VulkanMemoryAllocator")
add_subdirectory ("3rdparties/rigtorp/SPSCQueue")
add_subdirectory ("3rdparties/rigtorp/MPMCQueue")
add_subdirectory ("3rdparties/imgui")

# Include sub-projects.
add_subdirectory ("BRenderer")

add_subdirectory("EditorApp")