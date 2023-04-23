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

message("Searching Assimp...")
find_package(assimp REQUIRED)
if (assimp_FOUND)
    
    message("ASSIMP_ROOT_DIR: ${ASSIMP_ROOT_DIR}")
    message("ASSIMP_LIBRARIES: ${ASSIMP_LIBRARIES}")
    message("ASSIMP_LIBRARY_DIRS: ${ASSIMP_LIBRARY_DIRS}")
    message("ASSIMP_INCLUDE_DIRS: ${ASSIMP_INCLUDE_DIRS}")
    
else()
    message(FATAL_ERROR "Error: Unable to find Assimp package")
    return()
endif()

# Define the project files
set(BRenderer_SOURCE_FILES
"BRenderer.cpp" 

"Core/App.cpp"
"Core/LogSystem.cpp"
"Core/Window.cpp" 
"Core/WindowManager.cpp"
"Core/PerspectiveCamera.cpp"

"Geometry/Geometry.cpp"
"Geometry/Mesh2D.cpp"

"Scene/Components.cpp"
"Scene/Entity.cpp"
"Scene/Scene.cpp"

"Scene/Components/MaterialComponent.cpp"
"Scene/Components/Mesh3DComponent.cpp"
"Scene/Components/NodeComponent.cpp"
"Scene/Components/Transform3DComponent.cpp"

"Importer/Importer.cpp"

"Renderer/Descriptors.cpp"
"Renderer/DeviceBuffer.cpp"
"Renderer/DevicePipeline.cpp"
"Renderer/RenderDevice.cpp"
"Renderer/Renderer.cpp" 
"Renderer/RenderSurface.cpp"
"Renderer/SceneRenderer.cpp"
"Renderer/Shader.cpp" 
"Renderer/Swapchain.cpp"
"Renderer/VkInitializerHelper.cpp" 

"Files/FilesUtils.cpp" 
)

set(BRenderer_HEADER_FILES
"BRenderer.h" 

"Core/App.h"
"Core/LogSystem.h"
"Core/Window.h" 
"Core/WindowManager.h"
"Core/PerspectiveCamera.h"

"Geometry/Geometry.h"
"Geometry/Mesh2D.h"

"Scene/Components.h"
"Scene/Entity.h"
"Scene/Scene.h"

"Scene/Components/MaterialComponent.h"
"Scene/Components/Mesh3DComponent.h"
"Scene/Components/NodeComponent.h"
"Scene/Components/Transform3DComponent.h"

"Importer/Importer.h"

"Renderer/Descriptors.h"
"Renderer/DeviceBuffer.h"
"Renderer/DevicePipeline.h"
"Renderer/RenderDevice.h"
"Renderer/Renderer.h"
"Renderer/RenderSurface.h"
"Renderer/SceneRenderer.h"

"Renderer/Shader.h"
"Renderer/Swapchain.h"
"Renderer/VkInitializerHelper.h" 

"Files/FilesUtils.h"
) 

source_group(Renderer REGULAR_EXPRESSION "Renderer/.*")
source_group(Core REGULAR_EXPRESSION "Core/.*")
source_group(Geometry REGULAR_EXPRESSION "Geometry/.*")
source_group(Scene REGULAR_EXPRESSION "Scene/.*")
source_group(Scene\\Components REGULAR_EXPRESSION "Scene/Components/.*")
source_group(dependencies REGULAR_EXPRESSION "dependencies/.*")
source_group(Importer REGULAR_EXPRESSION "Importer/.*")
source_group("Files" REGULAR_EXPRESSION "Files/.*")

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

target_include_directories(BRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}" ${CMAKE_CURRENT_SOURCE_DIR} "dependencies" )

target_link_directories(BRenderer PUBLIC "${VULKAN_PATH}/Bin;${VULKAN_PATH}/Lib")

target_link_libraries(BRenderer PUBLIC "vulkan-1" "SDL2" "SDL2main" "${ASSIMP_LIBRARIES}" PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

if (TARGET spdlog)
    message("SpdLog Found!")
    target_compile_definitions(BRenderer PRIVATE USE_SPDLOG)
else()
    message("SpdLog if not working")
endif()

# Set MSVC working directory to the target executable directory.
if (MSVC AND CMAKE_VERSION VERSION_GREATER 3.8)
    set_property(TARGET BRenderer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:BRenderer>)
endif()

if (WIN32)
    add_custom_command(TARGET BRenderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  $<TARGET_FILE:assimp::assimp> $<TARGET_FILE_DIR:BRenderer>
    )

    add_custom_command(TARGET BRenderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSIMP_ROOT_DIR}/bin/pugixml.dll" $<TARGET_FILE_DIR:BRenderer>
    )

    add_custom_command(TARGET BRenderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSIMP_ROOT_DIR}/bin/zlib1.dll" $<TARGET_FILE_DIR:BRenderer>
    )

    add_custom_command(TARGET BRenderer POST_BUILD
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/Renderer/Shaders/compile_shaders.bat"
    )
endif()

add_custom_command(TARGET BRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Renderer/Shaders/vert.spv" $<TARGET_FILE_DIR:BRenderer>
)

add_custom_command(TARGET BRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Renderer/Shaders/frag.spv" $<TARGET_FILE_DIR:BRenderer>
)