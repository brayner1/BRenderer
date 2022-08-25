#ifndef BRR_VKINITIALIZERHELPER_H
#define BRR_VKINITIALIZERHELPER_H

namespace brr
{
	class Window;
}

namespace brr::render::VkHelpers
{

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> m_graphicsFamily;
		std::optional<uint32_t> m_presentFamily;
		std::optional<uint32_t> m_computeFamily;
		std::optional<uint32_t> m_transferFamily;
	};

	struct SwapChainProperties
	{
		vk::SurfaceCapabilitiesKHR m_surfCapabilities;
		std::vector<vk::SurfaceFormatKHR> m_surfFormats;
		std::vector<vk::PresentModeKHR> m_presentModes;
	};

	vk::PhysicalDevice Select_PhysDevice(std::vector<vk::PhysicalDevice>& physical_devices, vk::SurfaceKHR surface);

	QueueFamilyIndices Find_QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

	SwapChainProperties Query_SwapchainProperties(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

	vk::SurfaceFormatKHR Select_SwapchainFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats);
	vk::PresentModeKHR Select_SwapchainPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes);
	vk::Extent2D Select_SwapchainExtent(Window* window, const vk::SurfaceCapabilitiesKHR& surface_capabilities);

	vk::Bool32 Check_ValidationLayers(const std::vector<const char*>& check_names, const std::vector<vk::LayerProperties>& layers);

}

#endif