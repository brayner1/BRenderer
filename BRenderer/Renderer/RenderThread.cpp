#include "RenderThread.h"

#include <thread>
#include <barrier>

#include <rigtorp/SPSCQueue.h>

#include <Renderer/Allocators/SystemsOwner.h>
#include <Renderer/Internal/CmdList/RenderUpdateCmdGroup.h>
#include <Renderer/Internal/IdOwner.h>
#include <Renderer/Internal/WindowRenderer.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/SceneRenderer.h>

#include "Internal/CmdList/Executors/SceneRendererCmdListExecutor.h"
#include "Internal/CmdList/Executors/WindowCmdListExecutor.h"

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
static IdOwner<uint32_t> s_surface_id_generator;
static IdOwner<uint32_t> s_camera_id_generator;
static IdOwner<uint32_t> s_light_id_generator;

static std::allocator<void> s_cmd_data_allocator = std::allocator<void>();

static VulkanRenderDevice* s_render_device = nullptr;

static size_t s_main_frame_number = 0;

namespace
{
    void ExecuteUpdateCommands(RenderUpdateCmdGroup& render_update_cmds)
    {
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
        // Add 'FRAME_LAG - 2' available update cmd sets.
        // One is already owned by Render Thread and the other by the Main Thread.
        for (uint32_t idx = 0; idx < (FRAME_LAG - 2); idx++)
        {
            s_available_update_queue.emplace();
        }
        RenderThreadLoop();

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
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateCameraProjectionCommand(
        camera_id, camera_fovy, camera_near, camera_far);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

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
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildDestroyEntityCommand(entity_id);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

void RenderThread::SceneRenderCmd_UpdateEntityTransform(uint64_t scene_id,
                                                        EntityID entity_id,
                                                        const glm::mat4& entity_transform)
{
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateEntityTransformCommand(entity_id, entity_transform);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

SurfaceID RenderThread::SceneRenderCmd_CreateSurface(uint64_t scene_id,
                                                     EntityID entity_id,
                                                     void* vertex_buffer_data,
                                                     size_t vertex_buffer_size,
                                                     void* index_buffer_data,
                                                     size_t index_buffer_size)
{
    SurfaceID surface_id           = SurfaceID(s_surface_id_generator.GetNewId());
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildCreateSurfaceCommand(
        entity_id, surface_id, vertex_buffer_data, vertex_buffer_size, index_buffer_data, index_buffer_size);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return surface_id;
}

void RenderThread::SceneRenderCmd_DestroySurface(uint64_t scene_id,
                                                 SurfaceID surface_id)
{
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildDestroySurfaceCommand(surface_id);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

void RenderThread::SceneRenderCmd_UpdateSurfaceVertexBuffer(uint64_t scene_id,
                                                            SurfaceID surface_id,
                                                            void* vertex_buffer_data,
                                                            size_t vertex_buffer_size)
{
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateSurfaceVertexBufferCommand(
        surface_id, vertex_buffer_data, vertex_buffer_size);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

void RenderThread::SceneRenderCmd_UpdateSurfaceIndexBuffer(uint64_t scene_id,
                                                           SurfaceID surface_id,
                                                           void* index_buffer_data,
                                                           size_t index_buffer_size)
{
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateSurfaceIndexBufferCommand(
        surface_id, index_buffer_data, index_buffer_size);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

LightID RenderThread::SceneRenderCmd_CreatePointLight(uint64_t scene_id,
                                                 const glm::vec3& position,
                                                 const glm::vec3& color,
                                                 float intensity)
{
    LightID light_id               = LightID(s_light_id_generator.GetNewId());
    SceneRendererCommand scene_cmd =
        SceneRendererCommand::BuildCreatePointLightCommand(light_id, position, color, intensity);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return light_id;
}

void RenderThread::SceneRenderCmd_UpdatePointLight(uint64_t scene_id,
                                              LightID light_id,
                                              const glm::vec3& position,
                                              const glm::vec3& color,
                                              float intensity)
{
    SceneRendererCommand scene_cmd =
        SceneRendererCommand::BuildUpdatePointLightCommand(light_id, position, color, intensity);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

LightID RenderThread::SceneRenderCmd_CreateDirectionalLight(uint64_t scene_id,
                                                       const glm::vec3& direction,
                                                       const glm::vec3& color,
                                                       float intensity)
{
    LightID light_id               = LightID(s_light_id_generator.GetNewId());
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildCreateDirectionalLightCommand(
        light_id, direction, color, intensity);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return light_id;
}

void RenderThread::SceneRenderCmd_UpdateDirectionalLight(uint64_t scene_id,
                                                    LightID light_id,
                                                    const glm::vec3& direction,
                                                    const glm::vec3& color,
                                                    float intensity)
{
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateDirectionalLightCommand(
        light_id, direction, color, intensity);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

LightID RenderThread::SceneRenderCmd_CreateSpotLight(uint64_t scene_id,
                                                const glm::vec3& position,
                                                float cutoff_angle,
                                                const glm::vec3& direction,
                                                float intensity,
                                                const glm::vec3& color)
{
    LightID light_id               = LightID(s_light_id_generator.GetNewId());
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildCreateSpotLightCommand(
        light_id, position, cutoff_angle, direction, intensity, color);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return light_id;
}

void RenderThread::SceneRenderCmd_UpdateSpotLight(uint64_t scene_id,
                                             LightID light_id,
                                             const glm::vec3& position,
                                             float cutoff_angle,
                                             const glm::vec3& direction,
                                             float intensity,
                                             const glm::vec3& color)
{
    SceneRendererCommand scene_cmd = SceneRendererCommand::BuildUpdateSpotLightCommand(
        light_id, position, cutoff_angle, direction, intensity, color);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
}

LightID RenderThread::SceneRenderCmd_CreateAmbientLight(uint64_t scene_id,
                                                   const glm::vec3& color,
                                                   float intensity)
{
    LightID light_id                     = LightID(s_light_id_generator.GetNewId());
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildCreateAmbientLightCommand(light_id, color, intensity);
    SceneRendererCmdList& scene_cmd_list = s_current_game_update_cmds.scene_cmd_list_map[scene_id];
    scene_cmd_list.push_back(scene_cmd);
    return light_id;
}

void RenderThread::SceneRenderCmd_UpdateAmbientLight(uint64_t scene_id,
                                                LightID light_id,
                                                const glm::vec3& color,
                                                float intensity)
{
    SceneRendererCommand scene_cmd       = SceneRendererCommand::BuildUpdateAmbientLightCommand(light_id, color, intensity);
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
