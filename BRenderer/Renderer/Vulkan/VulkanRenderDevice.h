#ifndef BRR_RENDERDEVICE_H
#define BRR_RENDERDEVICE_H
#include <Renderer/RenderDefs.h>
#include <Renderer/Descriptors.h>
#include <Renderer/DevicePipeline.h>
#include <Renderer/Shader.h>
#include <Renderer/Allocators/ResourceAllocator.h>
#include <Renderer/Allocators/StagingAllocator.h>
#include <Renderer/Vulkan/VkInitializerHelper.h>
#include <Renderer/Vulkan/VulkanInc.h>

#include "glm/vec2.hpp"

namespace brr::render
{
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
		vk::Semaphore EndFrame(vk::Semaphore wait_semaphore, vk::Fence wait_fence);

		uint32_t GetCurrentFrame() const { return m_current_frame; }

        /* Shader */

		Shader CreateShaderFromFiles(std::string vertex_file_name, std::string frag_file_name);

		/* Synchronization */

		void WaitIdle() const;

		/* Vulkan Objects (Instance, Device, CommandPools and Queues) */

		[[nodiscard]] FORCEINLINE vk::Instance Get_VkInstance() const { return m_vulkan_instance; }

		[[nodiscard]] FORCEINLINE vk::PhysicalDevice Get_VkPhysicalDevice() const { return m_phys_device; }
		[[nodiscard]] FORCEINLINE vk::Device Get_VkDevice() const { return m_device; }

		/**********
		 * Queues *
		 **********/

		[[nodiscard]] FORCEINLINE VkHelpers::QueueFamilyIndices GetQueueFamilyIndices() const { return m_queue_family_indices; }

		[[nodiscard]] FORCEINLINE vk::Queue GetGraphicsQueue() const { return m_graphics_queue; }
		[[nodiscard]] FORCEINLINE vk::Queue GetPresentQueue() const { return m_presentation_queue; }
		[[nodiscard]] FORCEINLINE vk::Queue GetTransferQueue() const { return m_transfer_queue; }

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

		vk::CommandBuffer GetCurrentGraphicsCommandBuffer();
		vk::CommandBuffer GetCurrentTransferCommandBuffer();

		/***************
		 * Descriptors *
		 ***************/

		[[nodiscard]] DescriptorLayoutBuilder GetDescriptorLayoutBuilder() const;
		[[nodiscard]] DescriptorSetBuilder<FRAME_LAG> GetDescriptorSetBuilder(const DescriptorLayout& layout) const;

		/**********
		 * Memory *
		 **********/

		enum MemoryUsage : int
		{
		    AUTO,
			AUTO_PREFER_DEVICE,
			AUTO_PREFER_HOST
		};

		/***********
		 * Buffers *
		 ***********/

		enum BufferUsage : int
		{
            TransferSrc                             = 1 << 0,
            TransferDst                             = 1 << 1,
            UniformTexelBuffer                      = 1 << 2,
            StorageTexelBuffer                      = 1 << 3,
            UniformBuffer                           = 1 << 4,
            StorageBuffer                           = 1 << 5,
            //IndexBuffer                             = 1 << 6,
            //VertexBuffer                            = 1 << 7,
            //IndirectBuffer                          = 1 << 8,
            //ShaderDeviceAddress                     = 1 << 9,
            //VideoDecodeSrc                          = 1 << 10,
            //VideoDecodeDst                          = 1 << 11,
            //TransformFeedbackBuffer                 = 1 << 12,
            //TransformFeedbackCounterBuffer          = 1 << 13,
            //ConditionalRendering                    = 1 << 14,
            //AccelerationStructureBuildInputReadOnly = 1 << 15,
            //AccelerationStructureStorage            = 1 << 16,
            //ShaderBindingTable                      = 1 << 17,
            //RayTracingNV                            = 1 << 18
		};

        BufferHandle CreateBuffer(size_t buffer_size, BufferUsage buffer_usage,
                                  MemoryUsage memory_usage, VmaAllocationCreateFlags buffer_allocation_flags);

		bool DestroyBuffer(BufferHandle buffer_handle);

		void* MapBuffer(BufferHandle buffer_handle);
		void UnmapBuffer(BufferHandle buffer_handle);

		bool UploadBufferData(BufferHandle dst_buffer_handle, void* data, size_t size, uint32_t offset);

        bool CopyBuffer(BufferHandle src_buffer_handle, BufferHandle dst_buffer_handle, size_t size,
                        uint32_t src_buffer_offset = 0, uint32_t dst_buffer_offset = 0, bool use_transfer_queue = true);

        bool CopyBuffer_Immediate(BufferHandle src_buffer, BufferHandle dst_buffer, size_t size,
                                  uint32_t src_buffer_offset = 0, uint32_t dst_buffer_offset = 0);

		[[nodiscard]] vk::DescriptorBufferInfo GetBufferDescriptorInfo(BufferHandle buffer_handle, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

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
        enum ImageUsage : int
        {
            TransferSrcImage			= (1 << 0),
            TransferDstImage			= (1 << 1),
            SampledImage				= (1 << 2),
            StorageImage				= (1 << 3),
            ColorAttachmentImage		= (1 << 4),
            DepthStencilAttachmentImage = (1 << 5),
            TransientAttachmentImage	= (1 << 6),
            InputAttachmentImage		= (1 << 7)
        };

        
        // 
        Texture2DHandle Create_Texture2D(size_t width, size_t height, ImageUsage image_usage);

		bool DestroyTexture2D(Texture2DHandle texture2d_handle);

        bool UpdateTexture2DData(Texture2DHandle texture2d_handle, const void* data, size_t buffer_size,
                                 const glm::ivec2& image_offset, const glm::uvec2& image_extent);

		vk::DescriptorImageInfo GetImageDescriptorInfo(Texture2DHandle texture2d_handle);

		/*********************
		 * Graphics Pipeline *
		 *********************/

		bool Create_GraphicsPipeline(Swapchain* swapchain);

		void Bind_GraphicsPipeline();
		void Bind_DescriptorSet(vk::DescriptorSet descriptor_set, uint32_t set_index);

		/************
		 * Commands *
		 ************/

		void Draw(uint32_t num_vertex, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance);
		void DrawIndexed(uint32_t num_indices, uint32_t num_instances, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);

	protected:

		void UpdateBufferData(vk::Buffer dst_buffer, void* data, size_t size, uint32_t src_offset, uint32_t dst_offset);

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
		void Init_DescriptorLayouts();
		void Init_Texture2DSampler();

		/***************************
		 * CommandBuffer Functions *
		 ***************************/

		[[nodiscard]] vk::Result BeginGraphicsCommandBuffer(vk::CommandBuffer graphics_cmd_buffer);
		[[nodiscard]] vk::Result BeginTransferCommandBuffer(vk::CommandBuffer transfer_cmd_buffer);

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

			uint32_t width, height;
			vk::DeviceSize buffer_size {};
			IndexType buffer_format {};
		};

		ResourceAllocator<Texture2D> m_texture2d_alloc;
		
	private: // Data

		friend class StagingAllocator;

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

			typedef std::pair<vk::Buffer, VmaAllocation> BufferDeleteElem;
			struct TextureDeleteElem
			{
			    vk::Image image;
				vk::ImageView image_view;
				VmaAllocation allocation;
			};
			std::vector<BufferDeleteElem> buffer_delete_list;
			std::vector<TextureDeleteElem> texture_delete_list;

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
		vk::DescriptorSetLayout m_descriptor_set0_layout {};
		vk::DescriptorSetLayout m_descriptor_set1_layout {};

		vk::Sampler m_texture2DSampler {};

		// DevicePipeline
		std::unique_ptr<render::DevicePipeline> m_graphics_pipeline {};

		std::unique_ptr<DescriptorAllocator> m_descriptor_allocator = nullptr;
		std::unique_ptr<DescriptorLayoutCache> m_descriptor_layout_cache = nullptr;

	};

	inline VulkanRenderDevice::BufferUsage operator|(VulkanRenderDevice::BufferUsage a, VulkanRenderDevice::BufferUsage b)
    {
        return static_cast<VulkanRenderDevice::BufferUsage>(static_cast<int>(a) | static_cast<int>(b));
    }

	inline VulkanRenderDevice::VertexFormatFlags operator|(VulkanRenderDevice::VertexFormatFlags a, VulkanRenderDevice::VertexFormatFlags b)
    {
        return static_cast<VulkanRenderDevice::VertexFormatFlags>(static_cast<int>(a) | static_cast<int>(b));
    }

	inline VulkanRenderDevice::ImageUsage operator|(VulkanRenderDevice::ImageUsage a, VulkanRenderDevice::ImageUsage b)
    {
        return static_cast<VulkanRenderDevice::ImageUsage>(static_cast<int>(a) | static_cast<int>(b));
    }

#define VKRD VulkanRenderDevice
}

#endif