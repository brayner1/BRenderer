#ifndef BRR_RENDERTHREAD_H
#define BRR_RENDERTHREAD_H

#include <Core/thirdpartiesInc.h>

#include <Renderer/RenderEnums.h>
#include <Renderer/RenderingResourceIDs.h>
#include <Renderer/SceneObjectsIDs.h>

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

        static void WindowRenderCmd_InitializeWindowRenderer(SDL_Window* window_handle);
        static void WindowRenderCmd_DestroyWindowRenderer(SDL_Window* window_handle);
        static void WindowRenderCmd_SurfaceLost(SDL_Window* window_handle);
        static void WindowRenderCmd_Resize(SDL_Window* window_handle);
        static void WindowRenderCmd_SetSceneView(SDL_Window* window_handle, uint64_t scene_id, CameraID camera_id);

        /***************************
         * Scene Renderer Commands *
         ***************************/

        static uint64_t RenderCmd_InitializeSceneRenderer();
        static void RenderCmd_DestroySceneRenderer(uint64_t scene_id);

        /*******************
         * Camera Commands *
         *******************/

        static CameraID SceneRenderCmd_CreateCamera(uint64_t scene_id,
                                                    EntityID owner_entity,
                                                    float camera_fovy,
                                                    float camera_near,
                                                    float camera_far);
        static void SceneRenderCmd_DestroyCamera(uint64_t scene_id, CameraID camera_id);
        static void SceneRenderCmd_UpdateCameraProjection(uint64_t scene_id,
                                                          CameraID camera_id,
                                                          float camera_fovy,
                                                          float camera_near,
                                                          float camera_far);

        /*******************
         * Entity Commands *
         *******************/

        static EntityID SceneRenderCmd_CreateEntity(uint64_t scene_id, const glm::mat4& entity_transform = glm::mat4());
        static void SceneRenderCmd_DestroyEntity(uint64_t scene_id, EntityID entity_id);
        static void SceneRenderCmd_UpdateEntityTransform(uint64_t scene_id, EntityID entity_id, const glm::mat4& entity_transform);

        static void SceneRenderCmd_AppendSurfaceToEntity(uint64_t scene_id, EntityID entity_id, SurfaceID surface_id);

        /**********************
         * Texture2D Commands *
         **********************/

        static TextureID ResourceCmd_CreateTexture2D(const void* image_data, uint32_t width, uint32_t height, DataFormat image_format);
        static void ResourceCmd_DestroyTexture2D(TextureID texture_id);

        /********************
         * Surface Commands *
         ********************/

        static SurfaceID ResourceCmd_CreateSurface(void* vertex_buffer_data,
                                                   size_t vertex_buffer_size,
                                                   void* index_buffer_data,
                                                   size_t index_buffer_size);

        static void ResourceCmd_DestroySurface(SurfaceID surface_id);

        static void ResourceCmd_UpdateSurfaceVertexBuffer(SurfaceID surface_id,
                                                          void* vertex_buffer_data,
                                                          size_t vertex_buffer_size);

        static void ResourceCmd_UpdateSurfaceIndexBuffer(SurfaceID surface_id,
                                                         void* index_buffer_data,
                                                         size_t index_buffer_size);

        /*******************
         * Lights Commands *
         *******************/

        static LightID SceneRenderCmd_CreatePointLight(uint64_t scene_id, const glm::vec3& position, const glm::vec3& color, float intensity);

        static void SceneRenderCmd_UpdatePointLight(uint64_t scene_id, LightID light_id, const glm::vec3& position, const glm::vec3& color, float intensity);

        static LightID SceneRenderCmd_CreateDirectionalLight(uint64_t scene_id, const glm::vec3& direction, const glm::vec3& color, float intensity);

        static void SceneRenderCmd_UpdateDirectionalLight(uint64_t scene_id, LightID light_id, const glm::vec3& direction, const glm::vec3& color,
                                    float intensity);

        static LightID SceneRenderCmd_CreateSpotLight(uint64_t scene_id, const glm::vec3& position, float cutoff_angle, const glm::vec3& direction,
                                float intensity, const glm::vec3& color);

        static void SceneRenderCmd_UpdateSpotLight(uint64_t scene_id, LightID light_id, const glm::vec3& position, float cutoff_angle,
                             const glm::vec3& direction,
                             float intensity, const glm::vec3& color);

        static LightID SceneRenderCmd_CreateAmbientLight(uint64_t scene_id, const glm::vec3& color, float intensity);

        static void SceneRenderCmd_UpdateAmbientLight(uint64_t scene_id, LightID light_id, const glm::vec3& color, float intensity);

        static void SceneRenderCmd_DestroyLight(uint64_t scene_id, LightID light_id);
    };

}

#endif