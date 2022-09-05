# CMakeList.txt : CMake project for BRenderer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Search for the Vulkan library
message(STATUS "Searching Vulkan...")
find_package(Vulkan)

if (EXISTS "${Vulkan_INCLUDE_DIRS}")
	if (NOT Vulkan_FOUND)
		message(STATUS "Vulkan library not found! But include directories were found.")
	else()
		message(STATUS "Vulkan package found automatically!")
	endif()

	message(STATUS "Retrieving Vulkan root directory through include directory.")
	
	# Remove the "/Include" from the directory, obtaining the root vulkan path
	set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
	STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})

	message(STATUS "Vulkan root directory: ${VULKAN_PATH}")
else()
	message(FATAL_ERROR "Error: Unable to find Vulkan package")
	return()
endif()

# Define the project files
set(BRenderer_SOURCE_FILES
"BRenderer.cpp" 

"Core/App.cpp" 
"Core/Window.cpp" 
"Core/WindowManager.cpp"
"Core/PerspectiveCamera.cpp"

"Scene/Entity.cpp"
"Scene/Scene.cpp"
"Scene/Components.cpp"
#"Importer/Importer.cpp"

"Renderer/VkInitializerHelper.cpp" 
"Renderer/Shader.cpp" 
"Renderer/Renderer.cpp" 
"Renderer/Swapchain.cpp"
"Renderer/RenderDevice.cpp"
"Renderer/DeviceBuffer.cpp"
"Renderer/Geometry/Geometry.cpp"
"Renderer/Geometry/Mesh2D.cpp"

"Files/FilesUtils.cpp" 
 )

set(BRenderer_HEADER_FILES
"BRenderer.h" 

"Core/App.h"
"Core/Window.h" 
"Core/WindowManager.h"
"Core/PerspectiveCamera.h"

"Scene/Entity.h"
"Scene/Scene.h"
"Scene/Components.h"
#"Importer/Importer.h"

"Renderer/VkInitializerHelper.h" 
"Renderer/Shader.h"
"Renderer/Renderer.h"
"Renderer/Swapchain.h"
"Renderer/RenderDevice.h"
"Renderer/DeviceBuffer.h"
"Renderer/Geometry/Geometry.h"
"Renderer/Geometry/Mesh2D.h"

"Files/FilesUtils.h"  
 ) 


# Add source to this project's executable.
add_executable (BRenderer ${BRenderer_SOURCE_FILES} ${BRenderer_HEADER_FILES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BRenderer PROPERTY CXX_STANDARD 20)
endif()

if (WIN32)
	# Add any required preprocessor definitions here
	add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
endif()

target_precompile_headers(BRenderer PRIVATE 
	[["pch.h"]]
)

target_include_directories(BRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}" ${CMAKE_CURRENT_SOURCE_DIR} "dependencies" "dependencies/assimp/include")

target_link_directories(BRenderer PUBLIC "${VULKAN_PATH}/Bin;${VULKAN_PATH}/Lib;dependencies/assimp/lib/x64")

target_link_libraries(BRenderer "vulkan-1" "SDL2" "SDL2main" "assimp-vc140-mt")