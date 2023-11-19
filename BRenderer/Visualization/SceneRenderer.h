#ifndef BRR_SCENERENDERER_H
#define BRR_SCENERENDERER_H
#include <Core/Storage/ContiguousPool.h>
#include <Geometry/Geometry.h>
#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/RenderDefs.h>
#include <Renderer/Descriptors.h>
#include <Renderer/DevicePipeline.h>
#include <Renderer/Allocators/StagingAllocator.h>

#include <Scene/Scene.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Scene/Components/NodeComponent.h>

namespace brr::vis
{
    enum class SurfaceId : uint64_t
    {
        NULL_ID = -1
    };

    class SceneRenderer
    {
    public:

        SceneRenderer(Scene* scene);

        ~SceneRenderer();

        SurfaceId CreateNewSurface(Mesh3DComponent::SurfaceData& surface, const Entity& owner_entity);
        void RemoveSurface(SurfaceId surface_id);

        void BeginRender(uint32_t buffer_index, size_t current_frame);

        void UpdateDirtyInstances();

        void Render3D();

    private:

        struct UniformBufferObject
        {
            glm::mat4 projection_view{ 1.f };
        };

        struct Mesh3DUniform
        {
            glm::mat4 model_matrix;
        };

        struct RenderData
        {
            RenderData() : m_owner_node(nullptr)
            {}

            RenderData (NodeComponent* owner_node) : m_owner_node(owner_node)
            {}

            NodeComponent* m_owner_node;

            render::VertexBufferHandle m_vertex_buffer_handle {};
            render::IndexBufferHandle m_index_buffer_handle {};
            bool m_vertices_dirty = true;
            bool m_indices_dirty  = true;

            uint32_t num_vertices = 0, num_indices = 0;

            render::DescriptorLayout m_descriptor_layout;
            std::array<bool, render::FRAME_LAG> m_uniform_dirty { true };
            std::array<render::DeviceBuffer, render::FRAME_LAG>       m_uniform_buffers{}; // model transform
            std::array<vk::DescriptorSet, render::FRAME_LAG>  m_descriptor_sets{};
        };

        void SetupCameraUniforms();

        void CreateVertexBuffer(std::vector<Vertex3_PosColor>& vertex_buffer, RenderData& render_data);
        void CreateIndexBuffer(std::vector<uint32_t>& index_buffer, RenderData& render_data);
        void DestroyBuffers(RenderData& render_data);

        void Init_UniformBuffers(RenderData& render_data);

        Scene* m_scene;
        render::VulkanRenderDevice* m_render_device = nullptr;

        uint32_t m_current_buffer = 0;
        size_t m_current_frame = 0;

        struct CameraUniformInfo
        {
            std::array<render::DeviceBuffer, render::FRAME_LAG> m_uniform_buffers;
            std::array<vk::DescriptorSet, render::FRAME_LAG> m_descriptor_sets;
        } m_camera_uniform_info;

        ContiguousPool<RenderData> m_render_data;
        entt::basic_storage<uint32_t, RenderData> m_entt_render_data;
    };

}

#endif
