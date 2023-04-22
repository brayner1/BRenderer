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

        void UpdateRenderData(entt::registry& scene_registry, uint32_t buffer_index, glm::mat4& projection_view);

        void Render(vk::CommandBuffer cmd_buffer, uint32_t buffer_index, const DevicePipeline& render_pipeline);

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
            DeviceBuffer m_vertex_buffer{};
            DeviceBuffer m_index_buffer{};

            size_t num_vertices = 0, num_indices = 0;

            DescriptorLayout m_descriptor_layout;
            std::array<bool, FRAME_LAG> m_uniform_dirty { true };
            std::array<DeviceBuffer, FRAME_LAG>       m_uniform_buffers{}; // model transform
            std::array<vk::DescriptorSet, FRAME_LAG>  m_descriptor_sets{};
        };

        enum class SurfaceId : uint64_t {};

        void OnAddedMesh3D(entt::registry& registry, entt::entity entity);

        void CreateVertexBuffer(Mesh3DComponent::SurfaceData& surface_data, RenderData& render_data);
        void CreateIndexBuffer(Mesh3DComponent::SurfaceData& surface_data, RenderData& render_data);

        void Init_UniformBuffers(RenderData& render_data);

        struct CameraUniformInfo
        {
            std::array<DeviceBuffer, FRAME_LAG> m_uniform_buffers;
            std::array<vk::DescriptorSet, FRAME_LAG> m_descriptor_sets;
        } m_camera_uniform_info;

        RenderDevice* m_render_device = nullptr;

        entt::basic_registry<SurfaceId> m_render_registry;
    };

}

#endif
