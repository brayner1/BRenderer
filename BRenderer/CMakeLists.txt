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
    
    "Core/Threading/MainThread.cpp"
    "Core/Threading/ThreadPool.cpp"
    "Core/App.cpp"
    "Core/LogSystem.cpp"
    
    "Core/UUID.cpp"
    
    "Geometry/Geometry.cpp"
    
    "Scene/Entity.cpp"
    "Scene/Scene.cpp"
    
    "Scene/Components/EntityComponent.cpp"
    "Scene/Components/LightComponents.cpp"
    "Scene/Components/MaterialComponent.cpp"
    "Scene/Components/Mesh3DComponent.cpp"
    "Scene/Components/NodeComponent.cpp"
    "Scene/Components/PerspectiveCameraComponent.cpp"
    "Scene/Components/Transform3DComponent.cpp"
 
    "Visualization/Image.cpp"
    "Visualization/SceneRendererProxy.cpp"
    "Visualization/SceneView.cpp"
    "Visualization/Window.cpp" 
    "Visualization/WindowManager.cpp"
    
    "Importer/Importer.cpp"
    
    "Renderer/Allocators/StagingAllocator.cpp"
    "Renderer/Allocators/SystemsOwner.cpp"
    "Renderer/Internal/CmdList/Executors/SceneRendererCmdListExecutor.cpp"
    "Renderer/Internal/CmdList/Executors/WindowCmdListExecutor.cpp"
    "Renderer/Internal/WindowRenderer.cpp"
    "Renderer/Vulkan/VkDescriptors.cpp"
    "Renderer/Vulkan/VkInitializerHelper.cpp" 
    "Renderer/Vulkan/VmaImpl.cpp"
    "Renderer/Vulkan/VulkanRenderDevice.cpp"
    "Renderer/Descriptors.cpp"
    "Renderer/DeviceBuffer.cpp"
    "Renderer/DeviceImage.cpp"
    "Renderer/DevicePipeline.cpp"
    "Renderer/DeviceSwapchain.cpp"
    "Renderer/RenderThread.cpp"
    "Renderer/SceneRenderer.cpp"
    "Renderer/Shader.cpp" 
    
    "Files/FilesUtils.cpp" 
)

set(BRenderer_HEADER_FILES
    "BRenderer.h" 
    
    "Core/Events/Event.h"
    "Core/Storage/ContiguousPool.h"
    "Core/Storage/ResourceAllocator.h"
    "Core/Threading/Barrier.h" 
    "Core/Threading/MainThread.h" 
    "Core/Threading/Threading.h" 
    "Core/Threading/ThreadPool.h" 
    "Core/Threading/Work.h"
    "Core/App.h"
    "Core/LogSystem.h"
    
    "Core/thirdpartiesInc.h"
    "Core/UUID.h"
    
    "Geometry/Geometry.h"
    
    "Scene/Components.h"
    "Scene/Entity.h"
    "Scene/Scene.h" 
    "Scene/Components/EntityComponent.h"
    "Scene/Components/LightComponents.h"
    "Scene/Components/MaterialComponent.h"
    "Scene/Components/Mesh3DComponent.h"
    "Scene/Components/NodeComponent.h"
    "Scene/Components/PerspectiveCameraComponent.h"
    "Scene/Components/Transform3DComponent.h"

    "Visualization/Image.h"
    "Visualization/SceneRendererProxy.h"
    "Visualization/SceneView.h"
    "Visualization/Window.h" 
    "Visualization/WindowManager.h"
    
    "Importer/Importer.h"
    
    "Renderer/Allocators/StagingAllocator.h"
    "Renderer/Allocators/SystemsOwner.h"
    "Renderer/Internal/CmdList/Executors/SceneRendererCmdListExecutor.h"
    "Renderer/Internal/CmdList/Executors/WindowCmdListExecutor.h"
    "Renderer/Internal/CmdList/CmdList.h"
    "Renderer/Internal/CmdList/RenderUpdateCmdGroup.h"
    "Renderer/Internal/CmdList/SceneRendererCmdList.h"
    "Renderer/Internal/CmdList/WindowCmdList.h"
    "Renderer/Internal/IdOwner.h"
    "Renderer/Internal/WindowRenderer.h"
    "Renderer/Vulkan/VKDescriptors.h"
    "Renderer/Vulkan/VkInitializerHelper.h"
    "Renderer/Vulkan/VulkanInc.h"
    "Renderer/Vulkan/VulkanRenderDevice.h"
    "Renderer/Descriptors.h"
    "Renderer/DeviceBuffer.h"
    "Renderer/DeviceImage.h"
    "Renderer/DevicePipeline.h"
    "Renderer/DeviceSwapchain.h"
    "Renderer/RenderDefs.h"
    "Renderer/RenderEnums.h"
    "Renderer/RenderThread.h"
    "Renderer/SceneObjectsIDs.h"
    "Renderer/RenderResourcesHandles.h"
    "Renderer/Shader.h"
   
    "Files/FilesUtils.h"
) 

source_group(Renderer REGULAR_EXPRESSION "Renderer/.*")
source_group(Visualization REGULAR_EXPRESSION "Visualization/.*")
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
  message("Setting CXX Standard to C++20")
  set_property(TARGET BRenderer PROPERTY CXX_STANDARD 20)
  set_property(TARGET BRenderer PROPERTY CXX_STANDARD_REQUIRED ON)
endif()

if (WIN32)
    # Add any required preprocessor definitions here
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    add_definitions(-DNOMINMAX)
endif()

target_include_directories(BRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}" ${CMAKE_CURRENT_SOURCE_DIR} "dependencies" )

target_link_directories(BRenderer PUBLIC $<TARGET_LINKER_FILE_DIR:Vulkan::Vulkan>)

target_link_libraries(BRenderer PUBLIC "SDL2" "SDL2main" PRIVATE Vulkan::Headers assimp::assimp spdlog::spdlog VulkanMemoryAllocator SPSCQueue)

if (TARGET spdlog)
    message("SpdLog Found!")
    target_compile_definitions(BRenderer PRIVATE USE_SPDLOG)
else()
    message("SpdLog if not working")
endif()

if (TARGET VulkanMemoryAllocator)
    message("VMA Found!")
    target_compile_definitions(BRenderer PRIVATE USE_VMA)
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
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  $<TARGET_FILE:spdlog::spdlog> $<TARGET_FILE_DIR:BRenderer>
    )

    add_custom_command(TARGET BRenderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/../Resources $<TARGET_FILE_DIR:BRenderer>/Resources
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