#include "RenderThread.h"

#include <thread>
#include <barrier>

#include <rigtorp/SPSCQueue.h>

#include <Renderer/Allocators/SystemsOwner.h>
#include <Renderer/Internal/CmdList/RenderUpdateCmdGroup.h>
#include <Renderer/Internal/IdOwner.h>
#include <Renderer/Internal/WindowRenderer.h>
#include <Renderer/Storages/MeshStorage.h>
#include <Renderer/Storages/RenderStorageGlobals.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/SceneRenderer.h>

#include "Internal/CmdList/Executors/SceneRendererCmdListExecutor.h"
#include "Internal/CmdList/Executors/WindowCmdListExecutor.h"
#include "Internal/CmdList/Executors/ResourceCmdListExecutor.h"

#include "backends/imgui_impl_sdl2.h"

using namespace brr::render;
using namespace internal;

static std::thread s_rendering_thread = std::thread();

static std::atomic<bool> s_stop_rendering = false;

static rigtorp::SPSCQueue<RenderUpdateCmdGroup> s_available_update_queue{FRAME_LAG};
static rigtorp::SPSCQueue<RenderUpdateCmdGroup> s_render_update_queue{FRAME_LAG};
static RenderUpdateCmdGroup s_current_render_update_cmds; // RenderUpdateCmdGroup currently owned by render thread
static RenderUpdateCmdGroup s_current_game_update_cmds;   // RenderUpdateCmdGroup currently owned by game thread

static IdOwner<uint64_t> s_scene_id_generator;
static IdOwner<uint32_t> s_entity_id_generator;
static IdOwner<uint32_t> s_camera_id_generator;
static IdOwner<uint32_t> s_light_id_generator;

static std::allocator<void> s_cmd_data_allocator = std::allocator<void>();

static VulkanRenderDevice* s_render_device = nullptr;

static size_t s_main_frame_number = 0;

namespace
{
    void ExecuteUpdateCommands(RenderUpdateCmdGroup& render_update_cmds)
    {
        ResourceCmdListExecutor resource_cmd_list_executor (render_update_cmds.resource_cmd_list);
        resource_cmd_list_executor.ExecuteCmdList();
        render_update_cmds.resource_cmd_list.clear();

        SceneRendererCmdListExecutor scene_renderer_cmd_list_executor(render_update_cmds.scene_cmd_list_map);
        scene_renderer_cmd_list_executor.ExecuteCmdList();
        render_update_cmds.scene_cmd_list_map.clear();

        WindowCmdListExecutor window_cmd_list_executor(render_update_cmds.window_cmd_list);
        window_cmd_list_executor.ExecuteCmdList();
        render_update_cmds.window_cmd_list.clear();
    }

    void RenderThread_SyncUpdate()
    {
        s_available_update_queue.push(std::move(s_current_render_update_cmds));

        RenderUpdateCmdGroup* front;
        do
        {
            front = s_render_update_queue.front();
        }
        while (!front && !s_stop_rendering);
        if (!s_stop_rendering)
        {
            s_current_render_update_cmds = std::move(*front);

            s_render_update_queue.pop();
        }
    }


    void RenderThreadLoop()
    {
        while (!s_stop_rendering)
        {
            s_render_device->BeginFrame();

            ExecuteUpdateCommands(s_current_render_update_cmds);

            SystemOwner<WindowRenderer>& window_renderer_storage = SystemsStorage::GetWindowRendererStorage();
            for (const auto& window_it : window_renderer_storage)
            {
                window_it.second->RenderWindow(&s_current_render_update_cmds.imgui_draw_data_snapshot.DrawData);
            }

            s_render_device->EndFrame();

            RenderThread_SyncUpdate();
        }
    }

    void RenderThreadFunction()
    {
        // Initialization "Frame", used for initializing engine default resources.
        s_render_device->BeginFrame();
        RenderStorageGlobals::material_storage.InitializeDefaults();
        s_render_device->EndFrame();

        // Add 'FRAME_LAG - 2' available update cmd sets.
        // One is already owned by Render Thread and the other by the Main Thread.
        for (uint32_t idx = 0; idx < (FRAME_LAG - 2); idx++)
        {
            s_available_update_queue.emplace();
        }
        RenderThreadLoop();

        // Wait for the device to be idle before executing remaining commands.
        VKRD::GetSingleton()->WaitIdle();

        // Execute any remaining commands in the render update queue.
        // Some may be left if the main thread was faster than the render thread.
        // Including commands to destroy still active resources.
        uint32_t executed_finish_cmdlists = 0;
        while (RenderUpdateCmdGroup* front = s_render_update_queue.front())
        {
            s_current_render_update_cmds = std::move(*front);
            s_render_update_queue.pop();
            ExecuteUpdateCommands(s_current_render_update_cmds);
            executed_finish_cmdlists++;
        }
        // clear current render thread imgui snapshot, destroying its command list and buffers.
        s_current_render_update_cmds.imgui_draw_data_snapshot.Clear();

        BRR_LogDebug("RenderThread: Executed {} command lists on thread finalization.", executed_finish_cmdlists);

        RenderStorageGlobals::material_storage.DestroyDefaults();
        SystemsStorage::GetWindowRendererStorage().Clear();
        SystemsStorage::GetSceneRendererStorage().Clear();

        BRR_LogInfo("Stopping Rendering Thread. Destroying render device.");
        VKRD::DestroyRenderDevice();
    }
}

void RenderThread::InitializeRenderingThread(RenderAPI render_api,
                                             SDL_Window* main_window)
{
    VKRD::CreateRenderDevice(main_window);
    s_render_device    = VKRD::GetSingleton();
    s_rendering_thread = std::thread(RenderThreadFunction);
}

void RenderThread::StopRenderingThread()
{
    BRR_LogDebug("Stopping rendering thread.");
    s_stop_rendering = true;
    s_rendering_thread.join();

    // Clear available updates queue (rendering thread is closed)
    while (s_available_update_queue.front())
    {
        s_available_update_queue.pop();
    }
    // clear current game thread imgui snapshot, destroying its command list and buffers.
    s_current_game_update_cmds.imgui_draw_data_snapshot.Clear();

    assert(s_render_update_queue.empty() && "Render update queue should be empty");
    assert(s_available_update_queue.empty() && "Available update queue should be empty");

    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    BRR_LogDebug("Rendering thread joined.");
}

void RenderThread::MainThread_SyncUpdate()
{
    s_current_game_update_cmds.imgui_draw_data_snapshot.SnapUsingSwap(ImGui::GetDrawData(), ImGui::GetTime());

    s_render_update_queue.push(std::move(s_current_game_update_cmds));

    RenderUpdateCmdGroup* front;
    do
    {
        front = s_available_update_queue.front();
    }
    while (!front);
    s_current_game_update_cmds = std::move(*front);

    s_available_update_queue.pop();
    s_main_frame_number += 1;
}

glm::uvec2 GetSDLWindowDrawableSize(SDL_Window* window_handle)
{
    int width, height;
    if ((SDL_GetWindowFlags(window_handle) & SDL_WINDOW_MINIMIZED) != 0)
    {
        width = height = 0;
    }
    else
    {
        SDL_Vulkan_GetDrawableSize(window_handle, &width, &height);
    }
    return {width, height};
}

// =================================
// ======== Window Renderer ========
// =================================

void RenderThread::WindowRenderCmd_InitializeWindowRenderer(SDL_Window* window_handle)
{
    SwapchainWindowHandle swapchain_window_handle = VKRD::GetSingleton()->CreateSwapchainWindowHandle(window_handle);
    const uint32_t window_id                      = SDL_GetWindowID(window_handle);
    const WindowCommand window_cmd                = WindowCommand::BuildCreateWindowCommand(window_id,
                                                                             GetSDLWindowDrawableSize(window_handle), swapchain_window_handle);
    BRR_LogDebug("Pushing RenderCmd to initialize window. Window ID: {}", window_id);
    s_current_game_update_cmds.window_cmd_list.push_back(window_cmd);
}

void RenderThread::WindowRenderCmd_DestroyWindowRenderer(SDL_Window* window_handle)
{
    const uint32_t window_id = SDL_GetWindowID(window_handle);
    BRR_LogDebug("Pushing RenderCmd to destroy window. Window ID: {}", window_id);
    s_current_game_update_cmds.window_cmd_list.emplace_back(WindowCommand::BuildDestroyWindowCommand(window_id));
}

void RenderThread::WindowRenderCmd_SurfaceLost(SDL_Window* window_handle)
{
    SwapchainWindowHandle swapchain_window_handle = VKRD::GetSingleton()->CreateSwapchainWindowHandle(window_handle);
    const uint32_t window_id                      = SDL_GetWindowID(window_handle);
    const WindowCommand window_cmd                = WindowCommand::BuildSurfaceLostCommand(window_id,
                                                                            GetSDLWindowDrawableSize(window_handle), swapchain_window_handle);
    BRR_LogDebug("Pushing RenderCmd to update surface of window. Window ID: {}", window_id);
    s_current_game_update_cmds.window_cmd_list.push_back(window_cmd);
}

void RenderThread::WindowRenderCmd_Resize(SDL_Window* window_handle)
{
    const uint32_t window_id       = SDL_GetWindowID(window_handle);
    
    const WindowCommand window_cmd = WindowCommand::BuildWindowResizedCommand(window_id,
                                                                              GetSDLWindowDrawableSize(window_handle));
    BRR_LogDebug("Pushing RenderCmd to resize window. Window ID: {}", window_id);
    s_current_game_update_cmds.window_cmd_list.push_back(window_cmd);
}

void RenderThread::WindowRenderCmd_SetSceneView(SDL_Window* window_handle,
                                                uint64_t scene_id,
                                                CameraID camera_id)
{
    const uint32_t window_id       = SDL_GetWindowID(window_handle);
    const WindowCommand window_cmd = WindowCommand::BuildWindowSetSceneCommand(window_id, scene_id, camera_id);
    BRR_LogDebug("Pushing RenderCmd to set window scene. Window ID: {}. Scene ID: {}. Camera ID: {}", window_id, scene_id,
                 static_cast<uint32_t>(camera_id));
    s_current_game_update_cmds.window_cmd_list.push_back(window_cmd);
}

// ===============================
// ======== SceneRenderer ========
// ===============================

uint64_t RenderThread::RenderCmd_InitializeSceneRenderer()
{
    const uint64_t scene_id              = s_scene_id_generator.GetNewId();
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildCreateSceneRendererCommand();
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    BRR_LogDebug("Pushing RenderCmd to initialize SceneRenderer. Scene ID: {}", scene_id);
    scene_cmd_list.push_back(scene_cmd);
    return scene_id;
}

void RenderThread::RenderCmd_DestroySceneRenderer(uint64_t scene_id)
{
    BRR_LogDebug("Pushing RenderCmd to destroy SceneRenderer. Scene ID: {}", scene_id);
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildDestroySceneRendererCommand();
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

// ========================
// ======== Camera ========
// ========================

CameraID RenderThread::SceneRenderCmd_CreateCamera(uint64_t scene_id,
                                                   EntityID owner_entity,
                                                   float camera_fovy,
                                                   float camera_near,
                                                   float camera_far)
{
    const CameraID camera_id       = CameraID(s_camera_id_generator.GetNewId());
    BRR_LogDebug("Pushing RenderCmd to create SceneRenderer Camera. Scene ID: {}. Camera ID: {}", scene_id, static_cast<uint32_t>(camera_id));
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildCreateCameraCommand(
        camera_id, owner_entity, camera_fovy, camera_near, camera_far);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return camera_id;
}

void RenderThread::SceneRenderCmd_DestroyCamera(uint64_t scene_id,
                                                CameraID camera_id)
{
    BRR_LogDebug("Pushing RenderCmd to destroy SceneRenderer Camera. Scene ID: {}. Camera ID: {}", scene_id, static_cast<uint32_t>(camera_id));
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildDestroyCameraCommand(camera_id);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

void RenderThread::SceneRenderCmd_UpdateCameraProjection(uint64_t scene_id,
                                                         CameraID camera_id,
                                                         float camera_fovy,
                                                         float camera_near,
                                                         float camera_far)
{
    BRR_LogDebug("Pushing RenderCmd to update SceneRenderer Camera projection. Scene ID: {}. Camera ID: {}", scene_id, static_cast<uint32_t>(camera_id));
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateCameraProjectionCommand(
        camera_id, camera_fovy, camera_near, camera_far);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

// ========================
// ======== Entity ========
// ========================

EntityID RenderThread::SceneRenderCmd_CreateEntity(uint64_t scene_id,
                                                   const glm::mat4& entity_transform)
{
    EntityID entity_id                   = EntityID(s_entity_id_generator.GetNewId());
    BRR_LogDebug("Pushing RenderCmd to create SceneRenderer Entity. Scene ID: {}. Entity ID: {}", scene_id, static_cast<uint32_t>(entity_id));
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildCreateEntityCommand(entity_id, entity_transform);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return entity_id;
}

void RenderThread::SceneRenderCmd_DestroyEntity(uint64_t scene_id,
                                                EntityID entity_id)
{
    BRR_LogDebug("Pushing RenderCmd to destroy SceneRenderer Entity. Scene ID: {}. Entity ID: {}", scene_id, static_cast<uint32_t>(entity_id));
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildDestroyEntityCommand(entity_id);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

void RenderThread::SceneRenderCmd_UpdateEntityTransform(uint64_t scene_id,
                                                        EntityID entity_id,
                                                        const glm::mat4& entity_transform)
{
    BRR_LogDebug("Pushing RenderCmd to update SceneRenderer Entity transform. Scene ID: {}. Entity ID: {}", scene_id, static_cast<uint32_t>(entity_id));
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateEntityTransformCommand(entity_id, entity_transform);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

void RenderThread::SceneRenderCmd_AppendSurfaceToEntity(uint64_t scene_id,
                                                        EntityID entity_id,
                                                        SurfaceID surface_id)
{
    BRR_LogDebug("Pushing RenderCmd to append Surface to SceneRenderer Entity. Scene ID: {}. Entity ID: {}. Surface ID: {}", scene_id, static_cast<uint32_t>(entity_id), static_cast<size_t>(surface_id));
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildAppendSurfaceCommand(entity_id, surface_id);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

// ===========================
// ======== Texture2D ========
// ===========================

TextureID RenderThread::ResourceCmd_CreateTexture2D(const void* image_data,
                                                    uint32_t width,
                                                    uint32_t height,
                                                    DataFormat image_format)
{
    TextureID texture_id = RenderStorageGlobals::texture_storage.AllocateTexture();
    BRR_LogDebug("Pushing RenderCmd to create Texture2D. Texture2D ID: {}", static_cast<size_t>(texture_id));
    ResourceCommand resource_cmd = ResourceCommand::BuildCreateTexture2DCommand(
        texture_id, image_data, width, height, image_format);
    ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    resource_cmd_list.push_back(resource_cmd);
    return texture_id;
}

void RenderThread::ResourceCmd_DestroyTexture2D(TextureID texture_id)
{
    BRR_LogDebug("Pushing RenderCmd to destroy Texture2D. Texture2D ID: {}", static_cast<size_t>(texture_id));
    ResourceCommand resource_cmd = ResourceCommand::BuildDestroyTexture2DCommand(texture_id);
    ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    resource_cmd_list.push_back(resource_cmd);
}

// ==========================
// ======== Material ========
// ==========================

MaterialID RenderThread::ResourceCmd_CreateMaterial(vis::MaterialData material_data)
{
    MaterialID material_id = RenderStorageGlobals::material_storage.AllocateResource();
    MaterialProperties material_properties = BuildMaterialProperties(material_data);
    BRR_LogDebug("Pushing RenderCmd to create Material. Material ID: {}", static_cast<size_t>(material_id));
    ResourceCommand resource_cmd = ResourceCommand::BuildCreateMaterialCommand(material_id, material_properties);
    ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    resource_cmd_list.push_back(resource_cmd);
    return material_id;
}

void RenderThread::ResourceCmd_UpdateMaterialProperties(MaterialID material_id,
                                                        vis::MaterialData material_data)
{
    BRR_LogDebug("Pushing RenderCmd to update Material Properties. Material ID: {}", static_cast<size_t>(material_id));
    MaterialProperties material_properties = BuildMaterialProperties(material_data);
    ResourceCommand resource_cmd = ResourceCommand::BuildUpdateMaterialCommand(material_id, material_properties);
    ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    resource_cmd_list.push_back(resource_cmd);
}

void RenderThread::ResourceCmd_DestroyMaterial(MaterialID material_id)
{
    BRR_LogDebug("Pushing RenderCmd to destroy Material. Material ID: {}", static_cast<size_t>(material_id));
    ResourceCommand resource_cmd = ResourceCommand::BuildDestroyMaterialCommand(material_id);
    ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    resource_cmd_list.push_back(resource_cmd);
}

// =========================
// ======== Surface ========
// =========================

SurfaceID RenderThread::ResourceCmd_CreateSurface(void* vertex_buffer_data,
                                                  size_t vertex_buffer_size,
                                                  void* index_buffer_data,
                                                  size_t index_buffer_size,
                                                  MaterialID surface_material)
{
    SurfaceID surface_id = RenderStorageGlobals::mesh_storage.AllocateResource();
    BRR_LogDebug("Pushing RenderCmd to create Render Surface. Surface ID: {}", static_cast<size_t>(surface_id));
    ResourceCommand resource_cmd = ResourceCommand::BuildCreateSurfaceCommand(surface_id, vertex_buffer_data,
                                                                              vertex_buffer_size, index_buffer_data,
                                                                              index_buffer_size, surface_material);

    ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    resource_cmd_list.push_back(resource_cmd);
    return surface_id;
}

void RenderThread::ResourceCmd_DestroySurface(SurfaceID surface_id)
{
    BRR_LogDebug("Pushing RenderCmd to destroy Render Surface. Surface ID: {}", static_cast<size_t>(surface_id));
    ResourceCommand resource_cmd       = ResourceCommand::BuildDestroySurfaceCommand(surface_id);
    ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    resource_cmd_list.push_back(resource_cmd);
}

void RenderThread::ResourceCmd_UpdateSurfaceVertexBuffer(SurfaceID surface_id,
                                                         void* vertex_buffer_data,
                                                         size_t vertex_buffer_size)
{
    //ResourceCommand resource_cmd = ResourceCommand::BuildUpdateSurfaceVertexBufferCommand(
    //    surface_id, vertex_buffer_data, vertex_buffer_size);
    //ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    //resource_cmd_list.push_back(resource_cmd);
}

void RenderThread::ResourceCmd_UpdateSurfaceIndexBuffer(SurfaceID surface_id,
                                                        void* index_buffer_data,
                                                        size_t index_buffer_size)
{
    //ResourceCommand resource_cmd = ResourceCommand::BuildUpdateSurfaceIndexBufferCommand(
    //    surface_id, index_buffer_data, index_buffer_size);
    //ResourceCmdList& resource_cmd_list = s_current_game_update_cmds.resource_cmd_list;
    //resource_cmd_list.push_back(resource_cmd);
}

// ========================
// ======== Lights ========
// ========================

LightID RenderThread::SceneRenderCmd_CreatePointLight(uint64_t scene_id,
                                                      EntityID owner_entity_id,
                                                      const glm::vec3& color,
                                                      float intensity)
{
    LightID light_id               = LightID(s_light_id_generator.GetNewId());
    SceneRendererCommand scene_cmd =
        SceneRendererCommand::BuildCreatePointLightCommand(light_id, owner_entity_id, color, intensity);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return light_id;
}

LightID RenderThread::SceneRenderCmd_CreateDirectionalLight(uint64_t scene_id,
                                                            EntityID owner_entity_id,
                                                            const glm::vec3& color,
                                                            float intensity)
{
    LightID light_id               = LightID(s_light_id_generator.GetNewId());
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildCreateDirectionalLightCommand(
        light_id, owner_entity_id, color, intensity);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return light_id;
}

LightID RenderThread::SceneRenderCmd_CreateSpotLight(uint64_t scene_id,
                                                     EntityID owner_entity_id,
                                                     const glm::vec3& color,
                                                     float intensity,
                                                     float cutoff_angle)
{
    LightID light_id               = LightID(s_light_id_generator.GetNewId());
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildCreateSpotLightCommand(
        light_id, owner_entity_id, color, intensity, cutoff_angle);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return light_id;
}

LightID RenderThread::SceneRenderCmd_CreateAmbientLight(uint64_t scene_id,
                                                        EntityID owner_entity_id,
                                                        const glm::vec3& color,
                                                        float intensity)
{
    LightID light_id               = LightID(s_light_id_generator.GetNewId());
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildCreateAmbientLightCommand(
        light_id, owner_entity_id, color, intensity);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return light_id;
}

void RenderThread::SceneRenderCmd_UpdateLight(uint64_t scene_id,
                                              LightID light_id,
                                              const glm::vec3& color,
                                              float intensity,
                                              float cutoff_angle)
{
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateLightCommand(light_id, color, intensity, cutoff_angle);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

void RenderThread::SceneRenderCmd_DestroyLight(uint64_t scene_id,
                                               LightID light_id)
{
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildDestroyLightCommand(light_id);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}
