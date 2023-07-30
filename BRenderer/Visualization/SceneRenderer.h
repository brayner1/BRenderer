#ifndef BRR_SCENERENDERER_H
#define BRR_SCENERENDERER_H
#include <Geometry/Geometry.h>
#include <Renderer/VulkanInc.h>
#include <Renderer/VulkanRenderDevice.h>
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

        SurfaceId CreateNewSurface(Mesh3DComponent::SurfaceData& surface, const Entity& owner_entity);
        void UpdateSurfaceVertexBuffer(SurfaceId surface_id, std::vector<Vertex3_PosColor>& vertex_buffer, uint32_t buffer_offset = 0);
        void UpdateSurfaceIndexBuffer(SurfaceId surface_id, std::vector<uint32_t>& index_buffer, uint32_t buffer_offset = 0);
        //TODO: Add RemoveSurface function. It should add the render data to a delete-list in the current buffer, and delete it only when this buffer is rendering again (means the resources are not used anymore).

        void BeginRender(uint32_t buffer_index, size_t current_frame, vk::CommandBuffer graphics_command_buffer, vk::CommandBuffer transfer_command_buffer);

        void UpdateDirtyInstances();

        void Render3D(const render::DevicePipeline& render_pipeline);

        struct MeshDirty {};

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
            RenderData (NodeComponent* owner_node) : m_owner_node(owner_node)
            {}

            NodeComponent* m_owner_node;

            render::DeviceBuffer m_vertex_buffer{};
            render::DeviceBuffer m_index_buffer{};
            bool m_vertices_dirty = true;
            bool m_indices_dirty  = true;

            size_t num_vertices = 0, num_indices = 0;

            render::DescriptorLayout m_descriptor_layout;
            std::array<bool, render::FRAME_LAG> m_uniform_dirty { true };
            std::array<render::DeviceBuffer, render::FRAME_LAG>       m_uniform_buffers{}; // model transform
            std::array<vk::DescriptorSet, render::FRAME_LAG>  m_descriptor_sets{};
        };

        void SetupCameraUniforms();

        void CreateVertexBuffer(std::vector<Vertex3_PosColor>& vertex_buffer, RenderData& render_data);
        void CreateIndexBuffer(std::vector<uint32_t>& index_buffer, RenderData& render_data);
        void UpdateBufferData(render::DeviceBuffer& buffer, void* data, uint32_t size, uint32_t offset);

        render::StagingBufferHandle CreateStagingBuffer(size_t buffer_size, void* buffer_data);

        void Init_UniformBuffers(RenderData& render_data);

        Scene* m_scene;
        render::VulkanRenderDevice* m_render_device = nullptr;

        render::StagingAllocator m_staging_allocator;

        uint32_t m_current_buffer = 0;
        size_t m_current_frame = 0;

        vk::CommandBuffer m_current_graphics_cmd_buffer = nullptr;
        vk::CommandBuffer m_current_transfer_cmd_buffer = nullptr;

        struct CameraUniformInfo
        {
            std::array<render::DeviceBuffer, render::FRAME_LAG> m_uniform_buffers;
            std::array<vk::DescriptorSet, render::FRAME_LAG> m_descriptor_sets;
        } m_camera_uniform_info;

        std::unordered_map<SurfaceId, uint32_t> m_surfId_idx_map;
        std::vector<RenderData> m_render_data;
    };

}

#endif
