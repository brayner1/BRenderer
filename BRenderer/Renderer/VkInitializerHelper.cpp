#include "Renderer/VkInitializerHelper.h"

#include "Core/Window.h"

namespace brr::render::VkHelpers
{
	vk::PhysicalDevice Select_PhysDevice(std::vector<vk::PhysicalDevice>& physical_devices, vk::SurfaceKHR surface)
	{
		vk::PhysicalDevice result = VK_NULL_HANDLE;
		for (vk::PhysicalDevice& device : physical_devices)
		{
			SDL_Log("Checking if device '%s' supports application.", device.getProperties().deviceName.data());

			// Check for needed queue families
			{
				QueueFamilyIndices indices = Find_QueueFamilies(device, surface);

				if (!indices.m_graphicsFamily.has_value())
				{
					SDL_Log("Could not find graphics queue family support. Checking next device.");
					continue;
				}

				if (!indices.m_presentFamily.has_value())
				{
					SDL_Log("Could not find presentation queue family support. Checking next device.");
					continue;
				}
			}

			// Query if the device provides support for swapchain 
			{
				bool swapchain_support = false;

				std::vector<vk::ExtensionProperties>  device_extensions = device.enumerateDeviceExtensionProperties();
				for (vk::ExtensionProperties& extension_properties : device_extensions)
				{
					if (!strcmp(extension_properties.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
					{
						swapchain_support = true;
					}
				}

				if (!swapchain_support)
				{
					SDL_Log("Could not find swapchain extension support on the device. Checking next device.");
					continue;
				}
			}
			// Query if the swapchain provides the necessary present modes and surface formats
			{
				SwapChainProperties swapchain_properties = Query_SwapchainProperties(device, surface);

				if (swapchain_properties.m_presentModes.empty())
				{
					SDL_Log("Could not find available presentation modes for the swapchain. Exiting application.");
					exit(1);
				}

				if (swapchain_properties.m_surfFormats.empty())
				{
					SDL_Log("Could not find available surface formats for the swapchain. Exiting application.");
					exit(1);
				}
			}

			// TODO: make device selection based on its properties and features
			result = device;
			if (result.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				SDL_Log("Found dedicated GPU. Selecting it for processing.");
				break;
			}
		}

		if (!result)
		{
			SDL_Log("Could not find any device capable of running this application. Exiting application.");
			exit(1);
		}

		return result;
	}

	QueueFamilyIndices Find_QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
	{
		QueueFamilyIndices indices{};
		bool separate_presentation_family;
		bool full_queue_support;
		// Check for queue families
		std::vector<vk::QueueFamilyProperties> queue_props = physical_device.getQueueFamilyProperties();
		for (uint32_t i = 0; i < queue_props.size(); i++)
		{
			// Check graphics family support
			if (queue_props[i].queueFlags & vk::QueueFlagBits::eGraphics)
			{
				SDL_Log("Found Graphics Queue in Family %d", i);
				if (!indices.m_graphicsFamily.has_value())
				{
					indices.m_graphicsFamily = i;
				}
			}
			// Check surface support
			if (physical_device.getSurfaceSupportKHR(i, surface))
			{
				SDL_Log("Found Present Queue in Family %d", i);
				if (!indices.m_presentFamily.has_value()
					|| (indices.m_graphicsFamily.has_value() && indices.m_graphicsFamily.value() == i))
				{
					indices.m_presentFamily = i;
				}
			}
			// Check for compute support (Not required)
			if (queue_props[i].queueFlags & vk::QueueFlagBits::eCompute)
			{
				SDL_Log("Found Compute Queue in Family %d", i);
				if (!indices.m_computeFamily.has_value())
				{
					indices.m_computeFamily = i;
				}
			}
			if (queue_props[i].queueFlags & vk::QueueFlagBits::eTransfer)
			{
				SDL_Log("Found Transfer Queue in Family %d", i);
				if (!indices.m_transferFamily.has_value() || 
					(indices.m_graphicsFamily.has_value() && i != indices.m_graphicsFamily.value()))
				{
					indices.m_transferFamily = i;
				}
			}
			if (queue_props[i].queueFlags & vk::QueueFlagBits::eSparseBinding)
			{
				SDL_Log("Found Sparse Binding Queue in Family %d", i);
			}

			//full_queue_support = indices.m_graphicsFamily.has_value() && indices.m_presentFamily.has_value();
			//if (full_queue_support)
			//{
			//	separate_presentation_family = indices.m_graphicsFamily.value() != indices.m_presentFamily.value();
			//	// If the device already supports the necessary queue families (Graphics and Presentation), and both are of the same family, stop search.
			//	if (!separate_presentation_family)
			//	{
			//		break;
			//	}
			//}
		}

		return indices;
	}

	SwapChainProperties Query_SwapchainProperties(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
	{
		SwapChainProperties swap_chain_properties;

		auto getSurfCapabResult = physical_device.getSurfaceCapabilitiesKHR(surface);
		if (getSurfCapabResult.result != vk::Result::eSuccess)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not get SurfaceCapabilitiesKHR! Result code: %s.", vk::to_string(getSurfCapabResult.result).c_str());
			exit(1);
		}
		swap_chain_properties.m_surfCapabilities = getSurfCapabResult.value;

		 auto getSurfFormatsResult = physical_device.getSurfaceFormatsKHR(surface);
		 if (getSurfFormatsResult.result != vk::Result::eSuccess)
		 {
			 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not get SurfaceFormatKHR! Result code: %s.", vk::to_string(getSurfFormatsResult.result).c_str());
			 exit(1);
		 }
		 swap_chain_properties.m_surfFormats = getSurfFormatsResult.value;

		 auto getSurfPresentModesResult = physical_device.getSurfacePresentModesKHR(surface);
		 if (getSurfPresentModesResult.result != vk::Result::eSuccess)
		 {
			 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not get SurfacePresentModeKHR! Result code: %s.", vk::to_string(getSurfPresentModesResult.result).c_str());
			 exit(1);
		 }
		 swap_chain_properties.m_presentModes = getSurfPresentModesResult.value;

		return swap_chain_properties;
	}

	vk::SurfaceFormatKHR Select_SwapchainFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats)
	{
		for (const auto& availableFormat : available_formats) {
			if (availableFormat.format == vk::Format::eR8G8B8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return availableFormat;
			}
		}

		return available_formats[0];
	}

	vk::PresentModeKHR Select_SwapchainPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes)
	{
		for (const auto& availablePresentMode : available_present_modes) {
			if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
				return availablePresentMode;
			}
		}

		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D Select_SwapchainExtent(Window* window, const vk::SurfaceCapabilitiesKHR& surface_capabilities)
	{
		if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return surface_capabilities.currentExtent;
		}
		else
		{
			const glm::uvec2 desired_extent = window->GetWindowExtent();

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(desired_extent.x),
				static_cast<uint32_t>(desired_extent.y)
			};

			actualExtent.width = std::clamp(actualExtent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	vk::Bool32 Check_ValidationLayers(const std::vector<const char*>& check_layers, 
									  const std::vector<vk::LayerProperties>& layers, 
									  std::vector<const char*>& accepted_layers)
	{
		vk::Bool32 found = VK_FALSE;
		for (const auto& name : check_layers) {
			for (const auto& layer : layers) {
				if (!strcmp(name, layer.layerName)) {
					found = VK_TRUE;
					accepted_layers.push_back(name);
					break;
				}
			}
		}
		return found;
	}
}
