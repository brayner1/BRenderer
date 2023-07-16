#ifndef BRR_RENDERDEVICE_H
#define BRR_RENDERDEVICE_H
#include "Renderer/RenderDefs.h"
#include "Renderer/Descriptors.h"
#include "Renderer/Shader.h"
#include "Renderer/VkInitializerHelper.h"

namespace brr::render
{
	class VulkanRenderDevice
	{
	public:

		static void CreateRenderDevice(Window* window);

        static VulkanRenderDevice* GetSingleton();

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
		    Primary = vk::CommandBufferLevel::ePrimary,
			Secondary = vk::CommandBufferLevel::eSecondary
		};

		[[nodiscard]] vk::Result AllocateGraphicsCommandBuffer(CommandBufferLevel level, uint32_t cmd_buffer_count, std::vector<vk::CommandBuffer>& out_command_buffers) const;
		[[nodiscard]] vk::Result AllocatePresentCommandBuffer(CommandBufferLevel level, uint32_t cmd_buffer_count, std::vector<vk::CommandBuffer>& out_command_buffers) const;
		[[nodiscard]] vk::Result AllocateTransferCommandBuffer(CommandBufferLevel level, uint32_t cmd_buffer_count, std::vector<vk::CommandBuffer>& out_command_buffers) const;

		/***************
		 * Descriptors *
		 ***************/

		[[nodiscard]] DescriptorLayoutBuilder GetDescriptorLayoutBuilder() const;
		[[nodiscard]] DescriptorSetBuilder<FRAME_LAG> GetDescriptorSetBuilder(const DescriptorLayout& layout) const;

		/***********
		 * Buffers *
		 ***********/

		void Create_Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage,
			vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& buffer_memory);

		void Copy_Buffer_Immediate(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size,
			vk::DeviceSize src_buffer_offset = 0, vk::DeviceSize dst_buffer_offset = 0);

	private:

		VulkanRenderDevice(Window* main_window);

		void Init_VkInstance(Window* window);
		void Init_PhysDevice(vk::SurfaceKHR surface);
		void Init_Queues_Indices(vk::SurfaceKHR surface);
		void Init_Device();
		void Init_CommandPool();

		static std::unique_ptr<VulkanRenderDevice> device_;

		// Vulkan Instance

		vk::Instance vulkan_instance_ {};

		// Physical and Logical Devices

		vk::PhysicalDevice phys_device_ {};
		vk::Device m_device {};

		// Command Pools

		vk::CommandPool graphics_command_pool_ {};
		vk::CommandPool present_command_pool_ {};
		vk::CommandPool transfer_command_pool_ {};

		// Queue families indices and queues

		VkHelpers::QueueFamilyIndices queue_family_indices_{};
		vk::Queue graphics_queue_{};
		vk::Queue presentation_queue_{};
		vk::Queue transfer_queue_{};
		bool different_present_queue_ = false;
		bool different_transfer_queue_ = false;

		DescriptorAllocator* m_pDescriptorAllocator = nullptr;
		DescriptorLayoutCache* m_pDescriptorLayoutCache = nullptr;

	};

#define VKRD VulkanRenderDevice
}

#endif