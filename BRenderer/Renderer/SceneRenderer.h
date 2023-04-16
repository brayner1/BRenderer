#ifndef BRR_SCENERENDERER_H
#define BRR_SCENERENDERER_H
#include "Renderer/Renderer.h"

#include "Scene/Components/Mesh3DComponent.h"

namespace brr::render
{

    class SceneRenderer
    {
    public:

        SceneRenderer(entt::registry& registry);

        void UpdateRenderData(uint32_t buffer_index, const std::vector<DeviceBuffer>& camera_uniform_buffers);

        void Render(vk::CommandBuffer cmd_buffer, uint32_t buffer_index, const DevicePipeline& render_pipeline);

    private:

        struct RenderData
        {
            DeviceBuffer m_vertex_buffer{};
            DeviceBuffer m_index_buffer{};

            size_t num_vertices, num_indices;

            DescriptorLayout m_descriptor_layout;
            std::array<bool, FRAME_LAG> m_uniform_dirty {true, true};
            std::array<DeviceBuffer, FRAME_LAG> m_uniform_buffers{}; // model transform
            std::array<vk::DescriptorSet, FRAME_LAG>  m_descriptor_sets{};
        };

        enum class SurfaceId : uint64_t {};

        void OnAddedMesh3D(entt::registry& registry, entt::entity entity);

        void CreateVertexBuffer(Mesh3DComponent::SurfaceData& surface_data, RenderData& render_data);
        void CreateIndexBuffer(Mesh3DComponent::SurfaceData& surface_data, RenderData& render_data);

        void Init_UniformBuffers(RenderData& render_data);
        
        DeviceBuffer m_camera_uniform;

        RenderDevice* m_render_device = nullptr;

        entt::basic_registry<SurfaceId> m_render_registry;
        entt::registry* m_scene_registry = nullptr;
    };

}

#endif
