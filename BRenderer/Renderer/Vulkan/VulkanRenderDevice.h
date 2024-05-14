#ifndef BRR_RENDERDEVICE_H
#define BRR_RENDERDEVICE_H

#include <Core/Storage/ResourceAllocator.h>
#include <Renderer/Allocators/StagingAllocator.h>
#include <Renderer/Vulkan/VkInitializerHelper.h>
#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/RenderDefs.h>
#include <Renderer/RenderEnums.h>
#include <Renderer/RenderResourcesHandles.h>
#include <Renderer/Shader.h>

#include "glm/vec2.hpp"

namespace brr::vis
{
    class WindowRenderer;
}

namespace brr::render
{
    struct DescriptorSetBinding;
    struct DescriptorLayoutBindings;
    struct DescriptorLayout;
    class DescriptorSetAllocator;

    struct SwapchainWindowHandle
    {
        vk::SurfaceKHR vk_surface;
    };

    //TODO: Inherit from a base class RenderDevice. Support multiple APIs in the future.
    class VulkanRenderDevice
    {
    public:

        static void CreateRenderDevice(vis::Window* window);
        static void DestroyRenderDevice();

        static VulkanRenderDevice* GetSingleton();

        ~VulkanRenderDevice();

        /* Frame */

        uint32_t BeginFrame();
        void EndFrame();

        constexpr uint32_t GetCurrentFrame() const { return m_current_frame; }

        constexpr uint32_t GetCurrentFrameBufferIndex() const { return m_current_buffer; }

        /* Shader */

        /* Synchronization */

        void WaitIdle() const;

        /*************
         * Swapchain *
         *************/

        SwapchainHandle Swapchain_Create(vis::WindowRenderer* window_renderer, 
                                         SwapchainWindowHandle window_handle,
                                         glm::uvec2   drawable_size);

        void Swapchain_Recreate(SwapchainHandle swapchain_handle,
                                glm::uvec2      drawable_size);

        void Swapchain_Destroy(SwapchainHandle swapchain_handle);

        uint32_t Swapchain_AcquireNextImage(SwapchainHandle swapchain_handle);

        bool Swapchain_PresentCurrentImage(SwapchainHandle swapchain_handle);

        std::vector<Texture2DHandle> GetSwapchainImages(SwapchainHandle swapchain_handle);

        void Swapchain_BeginRendering(SwapchainHandle swapchain_handle, Texture2DHandle depth_image_handle = null_handle);
        void Swapchain_EndRendering(SwapchainHandle swapchain_handle);

        void RenderTarget_BeginRendering(Texture2DHandle color_attachment_handle, Texture2DHandle depth_attachment_handle, bool use_stencil = false);
        void RenderTarget_EndRendering(Texture2DHandle color_attachment_handle);

        [[nodiscard]] SwapchainWindowHandle CreateSwapchainWindowHandle(SDL_Window* window) const;

        /**********
         * Queues *
         **********/

        [[nodiscard]] FORCEINLINE VkHelpers::QueueFamilyIndices GetQueueFamilyIndices() const { return m_queue_family_indices; }

        [[nodiscard]] FORCEINLINE bool IsDifferentPresentQueue() const noexcept { return m_different_present_queue; }
        [[nodiscard]] FORCEINLINE bool IsDifferentTransferQueue() const noexcept { return m_different_transfer_queue; }

        /*******************
         * Command Buffers *
         *******************/

        enum class CommandBufferLevel : uint8_t
        {
            Primary = (uint8_t)vk::CommandBufferLevel::ePrimary,
            Secondary = (uint8_t)vk::CommandBufferLevel::eSecondary
        };

        /***************
         * Descriptors *
         ***************/

        DescriptorLayoutHandle CreateDescriptorSetLayout(const DescriptorLayoutBindings& descriptor_layout_bindings);

        std::vector<DescriptorSetHandle> AllocateDescriptorSet(DescriptorLayoutHandle descriptor_layout,
                                                               uint32_t               number_sets);

        bool UpdateDescriptorSetResources(DescriptorSetHandle descriptor_set_handle, const std::vector<DescriptorSetBinding>& shader_bindings);

        /**********
         * Memory *
         **********/

        /***********
         * Buffers *
         ***********/

        BufferHandle CreateBuffer(size_t buffer_size, BufferUsage buffer_usage,
                                  MemoryUsage memory_usage);

        bool DestroyBuffer(BufferHandle buffer_handle);

        void* MapBuffer(BufferHandle buffer_handle);
        void UnmapBuffer(BufferHandle buffer_handle);

        bool UploadBufferData(BufferHandle dst_buffer_handle, void* data, size_t size, uint32_t offset);

        bool CopyBuffer(BufferHandle src_buffer_handle, BufferHandle dst_buffer_handle, size_t size,
                        uint32_t src_buffer_offset = 0, uint32_t dst_buffer_offset = 0, bool use_transfer_queue = true);

        bool CopyBuffer_Immediate(BufferHandle src_buffer, BufferHandle dst_buffer, size_t size,
                                  uint32_t src_buffer_offset = 0, uint32_t dst_buffer_offset = 0);

        /******************
         * Vertex Buffers *
         ******************/

        enum class VertexFormatFlags : int
        {
            NORMAL    = 1 << 0,
            TANGENT   = 1 << 1,
            BITANGENT = 1 << 2,
            COLOR     = 1 << 3,
            UV0       = 1 << 4,
            UV1       = 1 << 5,
            ALL       = NORMAL | TANGENT | BITANGENT | COLOR | UV0 | UV1
        };

        VertexBufferHandle CreateVertexBuffer(size_t buffer_size, VertexFormatFlags format, void* data = nullptr);

        bool DestroyVertexBuffer(VertexBufferHandle vertex_buffer_handle);

        bool UpdateVertexBufferData(VertexBufferHandle vertex_buffer_handle, void* data, size_t data_size, uint32_t dst_offset);

        bool BindVertexBuffer(VertexBufferHandle vertex_buffer_handle);

        /*****************
         * Index Buffers *
         *****************/

        enum class IndexType : int
        {
            UINT8,
            UINT16,
            UINT32
        };

        IndexBufferHandle CreateIndexBuffer(size_t buffer_size, IndexType format, void* data = nullptr);

        bool DestroyIndexBuffer(IndexBufferHandle index_buffer_handle);

        bool UpdateIndexBufferData(IndexBufferHandle index_buffer_handle, void* data, size_t data_size, uint32_t dst_offset);

        bool BindIndexBuffer(IndexBufferHandle index_buffer_handle);

        /************
         * Textures *
         ************/
        
        // 
        Texture2DHandle Create_Texture2D(uint32_t width, uint32_t height, ImageUsage image_usage, DataFormat image_format);

        bool DestroyTexture2D(Texture2DHandle texture2d_handle);

        bool UpdateTexture2DData(Texture2DHandle texture2d_handle, const void* data, size_t buffer_size,
                                 const glm::ivec2& image_offset, const glm::uvec2& image_extent);

        void Texture2D_Blit(Texture2DHandle src_texture2d_handle, Texture2DHandle dst_texture2d_handle);

        /*********************
         * Graphics Pipeline *
         *********************/

        ResourceHandle Create_GraphicsPipeline(const Shader& shader,
                                               const std::vector<DataFormat>& color_attachment_formats,
                                               DataFormat depth_attachment_format);

        bool DestroyGraphicsPipeline(ResourceHandle graphics_pipeline_handle);

        void Bind_GraphicsPipeline(ResourceHandle graphics_pipeline_handle);
        void Bind_DescriptorSet(ResourceHandle graphics_pipeline_handle, DescriptorSetHandle descriptor_set_handle, uint32_t set_index);

        /************
         * Commands *
         ************/

        void Draw(uint32_t num_vertex, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance);
        void DrawIndexed(uint32_t num_indices, uint32_t num_instances, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);

    protected:

        void UpdateBufferData(vk::Buffer dst_buffer, void* data, size_t size, uint32_t src_offset, uint32_t dst_offset);

        static bool TransitionImageLayout(vk::CommandBuffer cmd_buffer, vk::Image image,
                                          vk::ImageLayout current_layout, vk::ImageLayout new_layout,
                                          vk::AccessFlags2 src_access_mask, vk::PipelineStageFlags2 src_stage_mask,
                                          vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 dst_stage_mask,
                                          vk::ImageAspectFlags image_aspect, uint32_t src_queue_index = 0, uint32_t dst_queue_index = 0);

        static void BufferMemoryBarrier(vk::CommandBuffer cmd_buffer, vk::Buffer buffer,
                                        size_t buffer_size, uint32_t buffer_offset,
                                        vk::PipelineStageFlags2 src_stage_flags, vk::AccessFlags2 src_access_flags,
                                        vk::PipelineStageFlags2 dst_stage_flags, vk::AccessFlags2 dst_access_flags,
                                        uint32_t src_queue_family = 0, uint32_t dst_queue_family = 0);

    private: // Initialization functions

        VulkanRenderDevice(vis::Window* main_window);

        /****************************
         * Initialization Functions *
         ****************************/

        void Init_VkInstance(vis::Window* window);
        void Init_PhysDevice(vk::SurfaceKHR surface);
        void Init_Queues_Indices(vk::SurfaceKHR surface);
        void Init_Device();
        void Init_Allocator();
        void Init_CommandPool();
        void Init_Frames();
        void Init_Texture2DSampler();

        /***********************
         * Swapchain Functions *
         ***********************/
        struct Swapchain;

        void Init_Swapchain(Swapchain& swapchain,
                            glm::uvec2 drawable_size);
        void Init_SwapchainResources(Swapchain& swapchain);
        void Init_SwapchainSynchronization(Swapchain& swapchain);

        void Cleanup_Swapchain(Swapchain& swapchain);

        /***********************
         * Frame Functions *
         ***********************/

        struct Frame;

        constexpr Frame& GetCurrentFrame() { return m_frames[m_current_buffer]; }

        /***************************
         * CommandBuffer Functions *
         ***************************/

        constexpr vk::CommandBuffer GetCurrentGraphicsCommandBuffer() { return GetCurrentFrame().graphics_cmd_buffer; }
        constexpr vk::CommandBuffer GetCurrentTransferCommandBuffer() { return GetCurrentFrame().transfer_cmd_buffer; }

        [[nodiscard]] vk::Result BeginCommandBuffer(vk::CommandBuffer cmd_buffer);

        [[nodiscard]] vk::Result SubmitGraphicsCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                              uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
                                                              vk::PipelineStageFlags* wait_dst_stages, uint32_t signal_semaphore_count,
                                                              vk::Semaphore* signal_semaphores, vk::Fence submit_fence);

        [[nodiscard]] vk::Result SubmitPresentCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                             uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
                                                             vk::PipelineStageFlags* wait_dst_stages,
                                                             uint32_t signal_semaphore_count, vk::Semaphore* signal_semaphores,
                                                             vk::Fence submit_fence);

        [[nodiscard]] vk::Result SubmitTransferCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                              uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
                                                              vk::PipelineStageFlags* wait_dst_stages,
                                                              uint32_t signal_semaphore_count, vk::Semaphore* signal_semaphores,
                                                              vk::Fence submit_fence);

    private: // Resources

        struct Buffer
        {
            vk::Buffer buffer {};
            VmaAllocation buffer_allocation {};
            VmaAllocationInfo allocation_info {};

            void* mapped = nullptr;

            vk::DeviceSize buffer_size {};
            vk::BufferUsageFlags buffer_usage {};
            VmaMemoryUsage memory_usage {};
        };

        ResourceAllocator<Buffer> m_buffer_alloc;

        struct VertexBuffer
        {
            vk::Buffer buffer {};
            VmaAllocation buffer_allocation {};
            VmaAllocationInfo allocation_info {};

            vk::DeviceSize buffer_size {};
            VertexFormatFlags buffer_format {};
        };

        ResourceAllocator<VertexBuffer> m_vertex_buffer_alloc;

        struct IndexBuffer
        {
            vk::Buffer buffer {};
            VmaAllocation buffer_allocation {};
            VmaAllocationInfo allocation_info {};

            vk::DeviceSize buffer_size {};
            IndexType buffer_format {};
        };

        ResourceAllocator<IndexBuffer> m_index_buffer_alloc;

        struct Texture2D
        {
            vk::Image image {};
            vk::ImageView image_view {};
            VmaAllocation image_allocation {};
            VmaAllocationInfo allocation_info {};

            vk::Extent2D image_extent {};
            vk::DeviceSize buffer_size {};
            vk::Format image_format {};
            vk::ImageLayout image_layout {};
        };

        ResourceAllocator<Texture2D> m_texture2d_alloc;

        struct GraphicsPipeline
        {
            vk::Pipeline pipeline {};
            vk::PipelineLayout pipeline_layout {};
        };

        ResourceAllocator<GraphicsPipeline> m_graphics_pipeline_alloc;

        struct DescriptorSet
        {
            vk::DescriptorSet descriptor_set {};
            vk::DescriptorPool set_pool {};
        };

        ResourceAllocator<DescriptorSet> m_descriptor_set_alloc;

        struct Swapchain
        {
            vis::WindowRenderer* window_renderer = nullptr;

            vk::SurfaceKHR surface {};

            vk::SwapchainKHR swapchain {};

            vk::Format swapchain_image_format {};
            vk::Format swapchain_depth_format {};
            vk::Extent2D swapchain_extent {};

            std::vector<Texture2DHandle> image_resources {};

            // Synchronization

            vk::Semaphore image_available_semaphores[FRAME_LAG];

            uint32_t current_image_idx  = 0;
            uint32_t current_buffer_idx = 0;
        };
        
        ResourceAllocator<Swapchain> m_swapchain_alloc;
        
    private: // Data
        friend class DeviceSwapchain;
        friend class Shader;
        friend class ShaderBuilder;
        friend class StagingAllocator;
        friend class DescriptorLayoutBuilder;
        friend class DescriptorSetUpdater;

        // Singleton Device
        static std::unique_ptr<VulkanRenderDevice> device_instance;

        // Vulkan Instance

        vk::Instance m_vulkan_instance {};

        // Physical and Logical Devices

        vk::PhysicalDevice m_phys_device {};
        vk::PhysicalDeviceProperties2 m_device_properties {};

        vk::Device m_device {};

        // Memory Allocator
        VmaAllocator m_vma_allocator {};

        // Staging Allocator

        StagingAllocator m_staging_allocator;

        // Command Buffers Pools

        vk::CommandPool m_graphics_command_pool {};
        vk::CommandPool m_present_command_pool {};
        vk::CommandPool m_transfer_command_pool {};

        struct Frame
        {
            vk::CommandBuffer transfer_cmd_buffer {};
            vk::CommandBuffer graphics_cmd_buffer {};

            vk::Semaphore transfer_finished_semaphore {};
            vk::Semaphore render_finished_semaphore {};
            vk::Fence     in_flight_fences {};
            std::vector<vk::Semaphore> images_available_semaphore {};

            struct SwapchainPresent
            {
                vk::PresentInfoKHR present_info;
                ResourceHandle swapchain_handle;
            };

            std::vector<SwapchainPresent> swapchain_present_infos {};

            typedef std::pair<vk::Buffer, VmaAllocation> BufferDeleteElem;
            struct TextureDeleteElem
            {
                vk::Image image;
                vk::ImageView image_view;
                VmaAllocation allocation;
            };
            std::vector<BufferDeleteElem> buffer_delete_list;
            std::vector<TextureDeleteElem> texture_delete_list;

            bool frame_in_progress = false;

            bool graphics_cmd_buffer_begin = false;
            bool transfer_cmd_buffer_begin = false;
        };

        void Free_FramePendingResources(Frame& frame);

        std::array<Frame, FRAME_LAG> m_frames {};

        uint32_t m_current_buffer = 0;
        uint32_t m_current_frame = 0;

        // Queue families indices and queues

        VkHelpers::QueueFamilyIndices m_queue_family_indices{};
        vk::Queue m_graphics_queue{};
        vk::Queue m_presentation_queue{};
        vk::Queue m_transfer_queue{};
        bool m_different_present_queue = false;
        bool m_different_transfer_queue = false;

        // Descriptor Sets
        vk::Sampler m_texture2DSampler {};

        std::unique_ptr<DescriptorSetAllocator> m_descriptor_allocator = nullptr;
        std::unique_ptr<DescriptorLayoutCache> m_descriptor_layout_cache = nullptr;

    };

    inline VulkanRenderDevice::VertexFormatFlags operator|(VulkanRenderDevice::VertexFormatFlags a, VulkanRenderDevice::VertexFormatFlags b)
    {
        return static_cast<VulkanRenderDevice::VertexFormatFlags>(static_cast<int>(a) | static_cast<int>(b));
    }

#define VKRD VulkanRenderDevice
}

#endif