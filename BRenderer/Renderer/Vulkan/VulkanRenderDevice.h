#ifndef BRR_RENDERDEVICE_H
#define BRR_RENDERDEVICE_H
#include <Renderer/RenderDefs.h>
#include <Renderer/Descriptors.h>
#include <Renderer/Shader.h>
#include <Renderer/Allocators/ResourceAllocator.h>
#include <Renderer/Vulkan/VkInitializerHelper.h>
#include <Renderer/Vulkan/VulkanInc.h>

namespace brr::render
{
	class VulkanRenderDevice
	{
	public:

		static void CreateRenderDevice(vis::Window* window);

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

		FORCEINLINE [[nodiscard]] vk::Instance Get_Instance() const { return vulkan_instance_; }

		FORCEINLINE [[nodiscard]] vk::PhysicalDevice Get_PhysicalDevice() const { return phys_device_; }
		FORCEINLINE [[nodiscard]] vk::Device Get_VkDevice() const { return m_device; }

		

		FORCEINLINE [[nodiscard]] VkHelpers::QueueFamilyIndices GetQueueFamilyIndices() const { return queue_family_indices_; }

		FORCEINLINE [[nodiscard]] vk::Queue GetGraphicsQueue() const { return graphics_queue_; }
		FORCEINLINE [[nodiscard]] vk::Queue GetPresentQueue() const { return presentation_queue_; }
		FORCEINLINE [[nodiscard]] vk::Queue GetTransferQueue() const { return transfer_queue_; }

		FORCEINLINE [[nodiscard]] bool IsDifferentPresentQueue() const { return different_present_queue_; }
		FORCEINLINE [[nodiscard]] bool IsDifferentTransferQueue() const { return different_transfer_queue_; }

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

		/***************
		 * Descriptors *
		 ***************/

		[[nodiscard]] DescriptorLayoutBuilder GetDescriptorLayoutBuilder() const;
		[[nodiscard]] DescriptorSetBuilder<FRAME_LAG> GetDescriptorSetBuilder(const DescriptorLayout& layout) const;

		/**********
		 * Memory *
		 **********/

		[[nodiscard]] VmaAllocator GetAllocator() const { return m_vma_allocator; }

		/***********
		 * Buffers *
		 ***********/

		enum BufferUsage : int
		{
		    TransferSrc = 1 << 0,
		    TransferDst = 1 << 1,
		    UniformTexelBuffer = 1 << 2,
		    StorageTexelBuffer = 1 << 3,
		    UniformBuffer = 1 << 4,
		    StorageBuffer = 1 << 5,
		    IndexBuffer = 1 << 6,
		    VertexBuffer = 1 << 7,
		    IndirectBuffer = 1 << 8,
		    ShaderDeviceAddress = 1 << 9,
		    VideoDecodeSrc = 1 << 10,
		    VideoDecodeDst = 1 << 11,
		    TransformFeedbackBuffer = 1 << 12,
		    TransformFeedbackCounterBuffer = 1 << 13,
		    ConditionalRendering = 1 << 14,
		    AccelerationStructureBuildInputReadOnly = 1 << 15,
		    AccelerationStructureStorage = 1 << 16,
		    ShaderBindingTable = 1 << 17,
		    RayTracingNV = 1 << 18
		};

        void Create_Buffer(size_t buffer_size, BufferUsage buffer_usage,
                           VmaMemoryUsage memory_usage, vk::Buffer& buffer, VmaAllocation& buffer_allocation,
                           VmaAllocationCreateFlags buffer_allocation_flags);

		void Copy_Buffer_Immediate(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size,
			vk::DeviceSize src_buffer_offset = 0, vk::DeviceSize dst_buffer_offset = 0);

	private:

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
		
		// Singleton Device
		static std::unique_ptr<VulkanRenderDevice> device_;

		// Vulkan Instance

		vk::Instance vulkan_instance_ {};

		// Physical and Logical Devices

		vk::PhysicalDevice phys_device_ {};
		vk::PhysicalDeviceProperties2 m_device_properties {};

		vk::Device m_device {};

		// Allocator
		VmaAllocator m_vma_allocator {};

		// Command Buffers Pools

		vk::CommandPool graphics_command_pool_ {};
		vk::CommandPool present_command_pool_ {};
		vk::CommandPool transfer_command_pool_ {};

		struct Frame
		{
			vk::CommandBuffer transfer_cmd_buffer {};
		    vk::CommandBuffer graphics_cmd_buffer {};

			vk::Semaphore transfer_finished_semaphore {};
			vk::Semaphore render_finished_semaphore {};
		};

		std::array<Frame, FRAME_LAG> m_frames {};

		uint32_t m_current_buffer = 0;
		uint32_t m_current_frame = 0;

		// Queue families indices and queues

		VkHelpers::QueueFamilyIndices queue_family_indices_{};
		vk::Queue graphics_queue_{};
		vk::Queue presentation_queue_{};
		vk::Queue transfer_queue_{};
		bool different_present_queue_ = false;
		bool different_transfer_queue_ = false;

		std::unique_ptr<DescriptorAllocator> m_pDescriptorAllocator = nullptr;
		std::unique_ptr<DescriptorLayoutCache> m_pDescriptorLayoutCache = nullptr;

	};

	inline VulkanRenderDevice::BufferUsage operator|(VulkanRenderDevice::BufferUsage a, VulkanRenderDevice::BufferUsage b)
    {
        return static_cast<VulkanRenderDevice::BufferUsage>(static_cast<int>(a) | static_cast<int>(b));
    }

#define VKRD VulkanRenderDevice
}

#endif