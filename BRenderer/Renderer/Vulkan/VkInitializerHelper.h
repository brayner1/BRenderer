#ifndef BRR_VKINITIALIZERHELPER_H
#define BRR_VKINITIALIZERHELPER_H
#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/RenderEnums.h>

#include <optional>
#include <vector>

namespace brr::vis
{
	class Window;
}

namespace brr::render::VkHelpers
{
	// Defines the queue families indices.
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> m_graphicsFamily;
		std::optional<uint32_t> m_presentFamily;
		std::optional<uint32_t> m_computeFamily;
		std::optional<uint32_t> m_transferFamily;
	};

	// Defines the relevant swapchain properties.
	struct SwapChainProperties
	{
		vk::SurfaceCapabilitiesKHR m_surfCapabilities;
		std::vector<vk::SurfaceFormatKHR> m_surfFormats;
		std::vector<vk::PresentModeKHR> m_presentModes;
	};

	// Select physical device to use. Prefer dedicated GPUs.
	vk::PhysicalDevice Select_PhysDevice(std::vector<vk::PhysicalDevice>& physical_devices, vk::SurfaceKHR surface);

	// Find the queue families indices for the given device.
	QueueFamilyIndices Find_QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

	// Obtain swapchain-related properties (surface capabilities, supported surface formats and present modes).
	SwapChainProperties Query_SwapchainProperties(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

	// Select preferred swapchain surface format given available formats.
	vk::SurfaceFormatKHR Select_SwapchainFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats);

    // Select the first candidate format that accepts the tiling and format features.
    vk::Format Select_SupportedFormat(vk::PhysicalDevice phys_device, std::vector<vk::Format> candidate_formats,
                                      vk::ImageTiling tiling, vk::FormatFeatureFlags features);

	// Search for the preferred present modes based on order of preference. If no preferred mode is available, then return FIFO present mode.
	vk::PresentModeKHR Select_SwapchainPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes, 
                                                   const std::vector<vk::PresentModeKHR>& preferred_present_modes);

    // Select the correct swapchain extent based on minimum and maximum extent bounds.
	vk::Extent2D Select_SwapchainExtent(vis::Window* window, const vk::SurfaceCapabilitiesKHR& surface_capabilities);

	//! Check if layers names are in check_layers and return the accepted layers. Returns true if at least one layer was accepted.
	vk::Bool32 CheckLayers(const std::vector<const char*>& check_layers,
									  const std::vector<vk::LayerProperties>& layers, 
									  std::vector<const char*>& accepted_layers);

	// Find the correct memory type index given the required memory properties.
	uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties, const vk::PhysicalDeviceMemoryProperties& phys_device_mem_properties);

	// Return the Vulkan Descriptor Type based on engine enum.
	vk::DescriptorType VkDescriptorTypeFromDescriptorType(DescriptorType descriptor_type);

	// Return the Vulkan shader stage flag based on engine enum.
	vk::ShaderStageFlags VkShaderStageFlagFromShaderStageFlag(ShaderStageFlag shader_stage_flag);

	// Return the VkFormat equivalent to the given DataFormat
	vk::Format VkFormatFromDeviceDataFormat(DataFormat data_format);

	// Return DataFormat equivalent to the given VkFormat
	DataFormat DataFormatFromVkFormat(vk::Format vk_format);

}

#endif