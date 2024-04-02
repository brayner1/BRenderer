#ifndef BRR_RENDERTHREAD_H
#define BRR_RENDERTHREAD_H

#include <Core/thirdpartiesInc.h>

namespace brr::render
{

    enum class RenderAPI
    {
        Vulkan
    };

    class RenderThread
    {
    public:

        static void InitializeRenderingThread(RenderAPI render_api, SDL_Window* main_window);

        static void StopRenderingThread();

        // Called from the main thread to signal the render thread that data update can begin.
        // It will not block the main thread, so you can continue doing work after calling this function,
        // but no rendering command should be called before call to `MainThread_SyncEndUpdate`
        static void MainThread_SyncUpdate();


        /*******************
         * Window Commands *
         *******************/

        static void RenderCmd_InitializeWindowRenderer(SDL_Window* window_handle, glm::uvec2 window_size);
        static void RenderCmd_DestroyWindowRenderer(SDL_Window* window_handle);
        static void RenderCmd_WindowSurfaceLost(SDL_Window* window_handle, glm::uvec2 window_size);
        static void RenderCmd_ResizeWindow(SDL_Window* window_handle, glm::uvec2 window_size);
        static void RenderCmd_SetWindowScene(SDL_Window* window_handle, uint64_t scene_id);

        /***************************
         * Scene Renderer Commands *
         ***************************/

        static void RenderCmd_InitializeSceneRenderer(uint64_t scene_id);
        static void RenderCmd_DestroySceneRenderer(uint64_t scene_id);

        // Camera commands
        static void RenderCmd_SceneCreateCamera(uint64_t scene_id,
                                                uint64_t camera_id,
                                                const glm::mat4& camera_view       = glm::mat4(),
                                                const glm::mat4& camera_projection = glm::mat4());
        static void RenderCmd_SceneUpdateCameraView(uint64_t scene_id, uint64_t camera_id, const glm::mat4& camera_view);
        static void RenderCmd_SceneUpdateCameraProjection(uint64_t scene_id, uint64_t camera_id, const glm::mat4& camera_projection);

        // Entity commands

        static void RenderCmd_SceneCreateEntity(uint64_t scene_id, uint64_t entity_id, const glm::mat4& entity_transform = glm::mat4());
        static void RenderCmd_SceneDestroyEntity(uint64_t scene_id, uint64_t entity_id);
        static void RenderCmd_SceneUpdateEntityTransform(uint64_t scene_id, uint64_t entity_id, const glm::mat4& entity_transform);

        // Surface commands

        static void RenderCmd_SceneCreateSurface(uint64_t scene_id,
                                                 uint64_t entity_id,
                                                 uint64_t surface_id,
                                                 void* vertex_buffer_data,
                                                 size_t vertex_buffer_size,
                                                 void* index_buffer_data,
                                                 size_t index_buffer_size);

        static void RenderCmd_SceneDestroySurface(uint64_t scene_id,
                                                  uint64_t entity_id,
                                                  uint64_t surface_id);

        static void RenderCmd_SceneUpdateSurfaceVertexBuffer(uint64_t scene_id,
                                                             uint64_t entity_id,
                                                             uint64_t surface_id,
                                                             void* vertex_buffer_data,
                                                             size_t vertex_buffer_size);

        static void RenderCmd_SceneUpdateSurfaceIndexBuffer(uint64_t scene_id,
                                                            uint64_t entity_id,
                                                            uint64_t surface_id,
                                                            void* index_buffer_data,
                                                            size_t index_buffer_size);

        /*******************
         * Lights Commands *
         *******************/


    };

}

#endif