#ifndef BRR_RENDERDEVICE_H
#define BRR_RENDERDEVICE_H
#include "VkInitializerHelper.h"

namespace brr::render
{

	class RenderDevice
	{
	public:

		RenderDevice(Window* main_window);

		/* Shader */

		void CreateShaderFromFilename(std::string file_name);

		/* Synchronization */

		void WaitIdle();

		/* Vulkan Objects (Instance, Device, Pools and Queues) */

		FORCEINLINE [[nodiscard]] vk::Instance Get_Instance() const { return vulkan_instance_; }

		FORCEINLINE [[nodiscard]] vk::PhysicalDevice Get_PhysicalDevice() const { return phys_device_; }
		FORCEINLINE [[nodiscard]] vk::Device Get_VkDevice() const { return device_; }

		FORCEINLINE [[nodiscard]] vk::CommandPool GetGraphicsCommandPool() const { return command_pool_; }
		FORCEINLINE [[nodiscard]] vk::CommandPool GetPresentCommandPool() const { return present_command_pool_; }
		FORCEINLINE [[nodiscard]] vk::CommandPool GetTransferCommandPool() const { return transfer_command_pool_; }

		FORCEINLINE [[nodiscard]] VkHelpers::QueueFamilyIndices GetQueueFamilyIndices() const { return queue_family_indices_; }

		FORCEINLINE [[nodiscard]] vk::Queue GetGraphicsQueue() const { return graphics_queue_; }
		FORCEINLINE [[nodiscard]] vk::Queue GetPresentQueue() const { return presentation_queue_; }
		FORCEINLINE [[nodiscard]] vk::Queue GetTransferQueue() const { return transfer_queue_; }

		FORCEINLINE [[nodiscard]] bool IsDifferentPresentQueue() const { return different_present_queue_; }
		FORCEINLINE [[nodiscard]] bool IsDifferentTransferQueue() const { return different_transfer_queue_; }

	private:

		void Init_VkInstance(Window* window);
		void Init_PhysDevice(vk::SurfaceKHR surface);
		void Init_Queues_Indices(vk::SurfaceKHR surface);
		void Init_Device();
		void Init_CommandPool();

		// Vulkan Instance

		vk::Instance vulkan_instance_ {};

		// Physical and Logical Devices

		vk::PhysicalDevice phys_device_ {};
		vk::Device device_ {};

		// Command Pools

		vk::CommandPool command_pool_ {};
		vk::CommandPool present_command_pool_ {};
		vk::CommandPool transfer_command_pool_ {};

		// Queue families indices and queues

		VkHelpers::QueueFamilyIndices queue_family_indices_{};
		vk::Queue graphics_queue_{};
		vk::Queue presentation_queue_{};
		vk::Queue transfer_queue_{};
		bool different_present_queue_ = false;
		bool different_transfer_queue_ = false;

	};

}

#endif