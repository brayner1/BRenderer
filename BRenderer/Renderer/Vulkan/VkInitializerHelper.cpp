#include "VkInitializerHelper.h"

#include <Core/LogSystem.h>
#include <Visualization/Window.h>


namespace brr::render::VkHelpers
{
	vk::PhysicalDevice Select_PhysDevice(std::vector<vk::PhysicalDevice>& physical_devices, vk::SurfaceKHR surface)
	{
		vk::PhysicalDevice result = VK_NULL_HANDLE;
		for (vk::PhysicalDevice& device : physical_devices)
		{
			BRR_LogInfo("Checking if device '{}' supports application.", device.getProperties().deviceName.data());

			// Check for needed queue families
			{
				QueueFamilyIndices indices = Find_QueueFamilies(device, surface);

				if (!indices.m_graphicsFamily.has_value())
				{
					BRR_LogInfo("Could not find graphics queue family support. Checking next device.");
					continue;
				}

				if (!indices.m_presentFamily.has_value())
				{
					BRR_LogInfo("Could not find presentation queue family support. Checking next device.");
					continue;
				}
			}

			// Query if the device provides support for swapchain 
			{
				bool swapchain_support = false;
				auto enumDeviceExtPropsResult = device.enumerateDeviceExtensionProperties();
				if (enumDeviceExtPropsResult.result != vk::Result::eSuccess)
				{
					BRR_LogError("Could not get SurfaceCapabilitiesKHR! Result code: {}.", vk::to_string(enumDeviceExtPropsResult.result).c_str());
					exit(1);
				}
				std::vector<vk::ExtensionProperties>  device_extensions = enumDeviceExtPropsResult.value;
				for (vk::ExtensionProperties& extension_properties : device_extensions)
				{
					if (!strcmp(extension_properties.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
					{
						swapchain_support = true;
					}
				}

				if (!swapchain_support)
				{
					BRR_LogInfo("Could not find swapchain extension support on the device. Checking next device.");
					continue;
				}
			}
			// Query if the swapchain provides the necessary present modes and surface formats
			{
				SwapChainProperties swapchain_properties = Query_SwapchainProperties(device, surface);

				if (swapchain_properties.m_presentModes.empty())
				{
					BRR_LogError("Could not find available presentation modes for the swapchain. Exiting application.");
					exit(1);
				}

				if (swapchain_properties.m_surfFormats.empty())
				{
					BRR_LogError("Could not find available surface formats for the swapchain. Exiting application.");
					exit(1);
				}
			}

			// TODO: make device selection based on its properties and features
			result = device;
			if (result.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				BRR_LogInfo("Found dedicated GPU. Selecting it for processing.");
				break;
			}
		}

		if (!result)
		{
			BRR_LogError("Could not find any device capable of running this application. Exiting application.");
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
		LogStreamBuffer log_msg = BRR_InfoStrBuff();
		log_msg << "Searching Queue Families.";
		for (uint32_t i = 0; i < queue_props.size(); i++)
		{
			log_msg << "\nFamily " << i << ": ";
			// Check graphics family support
			if (queue_props[i].queueFlags & vk::QueueFlagBits::eGraphics)
			{
				log_msg << "\n\tFound Graphics Queue";
				if (!indices.m_graphicsFamily.has_value())
				{
					indices.m_graphicsFamily = i;
				}
			}
			// Check surface support
			auto surfSupportKHRResult = physical_device.getSurfaceSupportKHR(i, surface);
			if (surfSupportKHRResult.result == vk::Result::eSuccess && surfSupportKHRResult.value)
			{
				log_msg << "\n\tFound Present Queue";
				if (!indices.m_presentFamily.has_value()
					|| (indices.m_graphicsFamily.has_value() && indices.m_graphicsFamily.value() == i))
				{
					indices.m_presentFamily = i;
				}
			}
			// Check for compute support (Not required)
			if (queue_props[i].queueFlags & vk::QueueFlagBits::eCompute)
			{
				log_msg << "\n\tFound Compute Queue";
				if (!indices.m_computeFamily.has_value())
				{
					indices.m_computeFamily = i;
				}
			}
			if (queue_props[i].queueFlags & vk::QueueFlagBits::eTransfer)
			{
				log_msg << "\n\tFound Transfer Queue";
				if (!indices.m_transferFamily.has_value() || 
					(indices.m_graphicsFamily.has_value()
					 && indices.m_transferFamily.value() == indices.m_graphicsFamily.value()
					 && i != indices.m_graphicsFamily.value()))
				{
					indices.m_transferFamily = i;
				}
			}
			if (queue_props[i].queueFlags & vk::QueueFlagBits::eSparseBinding)
			{
				log_msg << "\n\tFound Sparse Binding Queue";
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
			BRR_LogError("Could not get SurfaceCapabilitiesKHR! Result code: {}.", vk::to_string(getSurfCapabResult.result).c_str());
			exit(1);
		}
		swap_chain_properties.m_surfCapabilities = getSurfCapabResult.value;

		 auto getSurfFormatsResult = physical_device.getSurfaceFormatsKHR(surface);
		 if (getSurfFormatsResult.result != vk::Result::eSuccess)
		 {
			 BRR_LogError("Could not get SurfaceFormatKHR! Result code: {}.", vk::to_string(getSurfFormatsResult.result).c_str());
			 exit(1);
		 }
		 swap_chain_properties.m_surfFormats = getSurfFormatsResult.value;

		 auto getSurfPresentModesResult = physical_device.getSurfacePresentModesKHR(surface);
		 if (getSurfPresentModesResult.result != vk::Result::eSuccess)
		 {
			 BRR_LogError("Could not get SurfacePresentModeKHR! Result code: {}.", vk::to_string(getSurfPresentModesResult.result).c_str());
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

    vk::Format Select_SupportedFormat(vk::PhysicalDevice phys_device, std::vector<vk::Format> candidate_formats,
                                      vk::ImageTiling tiling, vk::FormatFeatureFlags features)
    {
		for (auto format : candidate_formats)
		{
		    vk::FormatProperties format_properties = phys_device.getFormatProperties(format);

			if (tiling == vk::ImageTiling::eLinear && (format_properties.linearTilingFeatures & features) == features)
			{
			    return format;
			}
			else if (tiling == vk::ImageTiling::eOptimal && (format_properties.optimalTilingFeatures & features) == features)
			{
			    return format;
			}
		}

		return vk::Format::eUndefined;
    }

    vk::PresentModeKHR Select_SwapchainPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes, 
                                                   const std::vector<vk::PresentModeKHR>& preferred_present_modes)
	{
		struct Selected
		{
		    uint32_t index {};
			vk::PresentModeKHR mode = vk::PresentModeKHR::eFifo;
		} selected_format;
		selected_format.index = preferred_present_modes.size();

		for (const auto& availablePresentMode : available_present_modes) 
		{
			for (uint32_t index = 0; index < selected_format.index; index++)
			{
			    if (availablePresentMode == preferred_present_modes[index])
			    {
			        selected_format.index = index;
					selected_format.mode  = availablePresentMode;
					break;
			    }
			}
			if (selected_format.index == 0)
			{
			    break;
			}
		}

		return selected_format.mode;
	}

	vk::Extent2D Select_SwapchainExtent(vis::Window* window, const vk::SurfaceCapabilitiesKHR& surface_capabilities)
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

	vk::Bool32 CheckLayers(const std::vector<const char*>& check_layers, 
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

    uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties,
        const vk::PhysicalDeviceMemoryProperties& phys_device_mem_properties)
    {
		for (uint32_t i = 0; i < phys_device_mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) &&
				(phys_device_mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find valid memory type.");
    }

    vk::DescriptorType VkDescriptorTypeFromDescriptorType(DescriptorType descriptor_type)
    {
		switch(descriptor_type)
		{
		case DescriptorType::UniformBuffer:
            return vk::DescriptorType::eUniformBuffer;
		case DescriptorType::StorageBuffer:
            return vk::DescriptorType::eStorageBuffer;
		case DescriptorType::CombinedImageSampler:
            return vk::DescriptorType::eCombinedImageSampler;
        case DescriptorType::SampledImage:
            return vk::DescriptorType::eSampledImage;
        case DescriptorType::StorageImage:
            return vk::DescriptorType::eStorageImage;
        case DescriptorType::Sampler:
            return vk::DescriptorType::eSampler;
        }
		return vk::DescriptorType::eUniformBuffer;
    }

    vk::ShaderStageFlags VkShaderStageFlagFromShaderStageFlag(ShaderStageFlag shader_stage_flag)
    {
        vk::ShaderStageFlags shader_stage;
		if  ((shader_stage_flag & ShaderStageFlag::VertexShader) != 0)
		{
			shader_stage |= vk::ShaderStageFlagBits::eVertex;
		}
		if  ((shader_stage_flag & ShaderStageFlag::FragmentShader) != 0)
		{
			shader_stage |= vk::ShaderStageFlagBits::eFragment;
		}
		if  ((shader_stage_flag & ShaderStageFlag::ComputeShader) != 0)
		{
			shader_stage |= vk::ShaderStageFlagBits::eCompute;
		}
		return shader_stage;
    }

    vk::Format VkFormatFromDeviceDataFormat(DataFormat data_format)
    {
		switch (data_format)
        {
        case DataFormat::R8_UNorm:
            return vk::Format::eR8Unorm;
        case DataFormat::R8_SNorm:
            return vk::Format::eR8Snorm;
        case DataFormat::R8_UScaled:
            return vk::Format::eR8Uscaled;
        case DataFormat::R8_SScaled:
            return vk::Format::eR8Sscaled;
        case DataFormat::R8_UInt:
            return vk::Format::eR8Uint;
        case DataFormat::R8_SInt:
            return vk::Format::eR8Sint;
        case DataFormat::R16_UNorm:
            return vk::Format::eR16Unorm;
        case DataFormat::R16_SNorm:
            return vk::Format::eR16Snorm;
        case DataFormat::R16_UScaled:
            return vk::Format::eR16Uscaled;
        case DataFormat::R16_SScaled:
            return vk::Format::eR16Sscaled;
        case DataFormat::R16_UInt:
            return vk::Format::eR16Uint;
        case DataFormat::R16_SInt:
            return vk::Format::eR16Sint;
        case DataFormat::R16_Float:
            return vk::Format::eR16Sfloat;
        case DataFormat::R8G8_UNorm:
            return vk::Format::eR8G8Unorm;
        case DataFormat::R8G8_SNorm:
            return vk::Format::eR8G8Snorm;
        case DataFormat::R8G8_UScaled:
            return vk::Format::eR8G8Uscaled;
        case DataFormat::R8G8_SScaled:
            return vk::Format::eR8G8Sscaled;
        case DataFormat::R8G8_UInt:
            return vk::Format::eR8G8Uint;
        case DataFormat::R8G8_SInt:
            return vk::Format::eR8G8Sint;
        case DataFormat::R8G8_SRGB:
            return vk::Format::eR8G8Srgb;
        case DataFormat::R8G8B8_UNorm:
            return vk::Format::eR8G8B8Unorm;
        case DataFormat::R8G8B8_SNorm:
            return vk::Format::eR8G8B8Snorm;
        case DataFormat::R8G8B8_UScaled:
            return vk::Format::eR8G8B8Uscaled;
        case DataFormat::R8G8B8_SScaled:
            return vk::Format::eR8G8B8Sscaled;
        case DataFormat::R8G8B8_UInt:
            return vk::Format::eR8G8B8Uint;
        case DataFormat::R8G8B8_SInt:
            return vk::Format::eR8G8B8Sint;
        case DataFormat::R8G8B8_SRGB:
            return vk::Format::eR8G8B8Srgb;
        case DataFormat::B8G8R8_UNorm:
            return vk::Format::eB8G8R8Unorm;
        case DataFormat::B8G8R8_SNorm:
            return vk::Format::eB8G8R8Snorm;
        case DataFormat::B8G8R8_UScaled:
            return vk::Format::eB8G8R8Uscaled;
        case DataFormat::B8G8R8_SScaled:
            return vk::Format::eB8G8R8Sscaled;
        case DataFormat::B8G8R8_UInt:
            return vk::Format::eB8G8R8Uint;
        case DataFormat::B8G8R8_SInt:
            return vk::Format::eB8G8R8Sint;
        case DataFormat::B8G8R8_SRGB:
            return vk::Format::eB8G8R8Srgb;
        case DataFormat::R32_UInt:
            return vk::Format::eR32Uint;
        case DataFormat::R32_SInt:
            return vk::Format::eR32Sint;
        case DataFormat::R32_Float:
            return vk::Format::eR32Sfloat;
        case DataFormat::R16G16_UNorm:
            return vk::Format::eR16G16Unorm;
        case DataFormat::R16G16_SNorm:
            return vk::Format::eR16G16Snorm;
        case DataFormat::R16G16_UScaled:
            return vk::Format::eR16G16Uscaled;
        case DataFormat::R16G16_SScaled:
            return vk::Format::eR16G16Sscaled;
        case DataFormat::R16G16_UInt:
            return vk::Format::eR16G16Uint;
        case DataFormat::R16G16_SInt:
            return vk::Format::eR16G16Sint;
        case DataFormat::R16G16_Float:
            return vk::Format::eR16G16Sfloat;
        case DataFormat::R8G8B8A8_UNorm:
            return vk::Format::eR8G8B8A8Unorm;
        case DataFormat::R8G8B8A8_SNorm:
            return vk::Format::eR8G8B8A8Snorm;
        case DataFormat::R8G8B8A8_UScaled:
            return vk::Format::eR8G8B8A8Uscaled;
        case DataFormat::R8G8B8A8_SScaled:
            return vk::Format::eR8G8B8A8Sscaled;
        case DataFormat::R8G8B8A8_UInt:
            return vk::Format::eR8G8B8A8Uint;
        case DataFormat::R8G8B8A8_SInt:
            return vk::Format::eR8G8B8A8Sint;
        case DataFormat::R8G8B8A8_SRGB:
            return vk::Format::eR8G8B8A8Srgb;
        case DataFormat::B8G8R8A8_UNorm:
            return vk::Format::eB8G8R8A8Unorm;
        case DataFormat::B8G8R8A8_SNorm:
            return vk::Format::eB8G8R8A8Snorm;
        case DataFormat::B8G8R8A8_UScaled:
            return vk::Format::eB8G8R8A8Uscaled;
        case DataFormat::B8G8R8A8_SScaled:
            return vk::Format::eB8G8R8A8Sscaled;
        case DataFormat::B8G8R8A8_UInt:
            return vk::Format::eB8G8R8A8Uint;
        case DataFormat::B8G8R8A8_SInt:
            return vk::Format::eB8G8R8A8Sint;
        case DataFormat::B8G8R8A8_SRGB:
            return vk::Format::eB8G8R8A8Srgb;
        case DataFormat::R16G16B16_UNorm:
            return vk::Format::eR16G16B16Unorm;
        case DataFormat::R16G16B16_SNorm:
            return vk::Format::eR16G16B16Snorm;
        case DataFormat::R16G16B16_UScaled:
            return vk::Format::eR16G16B16Uscaled;
        case DataFormat::R16G16B16_SScaled:
            return vk::Format::eR16G16B16Sscaled;
        case DataFormat::R16G16B16_UInt:
            return vk::Format::eR16G16B16Uint;
        case DataFormat::R16G16B16_SInt:
            return vk::Format::eR16G16B16Sint;
        case DataFormat::R16G16B16_Float:
            return vk::Format::eR16G16B16Sfloat;
        case DataFormat::R64_UInt:
            return vk::Format::eR64Uint;
        case DataFormat::R64_SInt:
            return vk::Format::eR64Sint;
        case DataFormat::R64_Float:
            return vk::Format::eR64Sfloat;
        case DataFormat::R32G32_UInt:
            return vk::Format::eR32G32Uint;
        case DataFormat::R32G32_SInt:
            return vk::Format::eR32G32Sint;
        case DataFormat::R32G32_Float:
            return vk::Format::eR32G32Sfloat;
        case DataFormat::R16G16B16A16_UNorm:
            return vk::Format::eR16G16B16A16Unorm;;
        case DataFormat::R16G16B16A16_SNorm:
            return vk::Format::eR16G16B16A16Snorm;
        case DataFormat::R16G16B16A16_UScaled:
            return vk::Format::eR16G16B16A16Uscaled;
        case DataFormat::R16G16B16A16_SScaled:
            return vk::Format::eR16G16B16A16Sscaled;
        case DataFormat::R16G16B16A16_UInt:
            return vk::Format::eR16G16B16A16Uint;
        case DataFormat::R16G16B16A16_SInt:
            return vk::Format::eR16G16B16A16Sint;
        case DataFormat::R16G16B16A16_Float:
            return vk::Format::eR16G16B16A16Sfloat;
        case DataFormat::R32G32B32_UInt:
            return vk::Format::eR32G32B32Uint;
        case DataFormat::R32G32B32_SInt:
            return vk::Format::eR32G32B32Sint;
        case DataFormat::R32G32B32_Float:
            return vk::Format::eR32G32B32Sfloat;
        case DataFormat::R64G64_UInt:
            return vk::Format::eR64G64Uint;
        case DataFormat::R64G64_SInt:
            return vk::Format::eR64G64Sint;
        case DataFormat::R64G64_Float:
            return vk::Format::eR64G64Sfloat;
        case DataFormat::R32G32B32A32_UInt:
            return vk::Format::eR32G32B32A32Uint;
        case DataFormat::R32G32B32A32_SInt:
            return vk::Format::eR32G32B32A32Sint;
        case DataFormat::R32G32B32A32_Float:
            return vk::Format::eR32G32B32A32Sfloat;
        case DataFormat::R64G64B64_UInt:
            return vk::Format::eR64G64B64Uint;
        case DataFormat::R64G64B64_SInt:
            return vk::Format::eR64G64B64Sint;
        case DataFormat::R64G64B64_Float:
            return vk::Format::eR64G64B64Sfloat;
        case DataFormat::R64G64B64A64_UInt:
            return vk::Format::eR64G64B64A64Uint;
        case DataFormat::R64G64B64A64_SInt:
            return vk::Format::eR64G64B64A64Sint;
        case DataFormat::R64G64B64A64_Float:
            return vk::Format::eR64G64B64A64Sfloat;
        case DataFormat::D16_UNorm:
            return vk::Format::eD16Unorm;
        case DataFormat::D32_Float:
            return vk::Format::eD32Sfloat;
        case DataFormat::S8_UInt:
            return vk::Format::eS8Uint;
        case DataFormat::D16_UNorm_S8_UInt:
            return vk::Format::eD16UnormS8Uint;
        case DataFormat::D24_UNorm_S8_UInt:
            return vk::Format::eD24UnormS8Uint;
        case DataFormat::D32_Float_S8_UInt:
            return vk::Format::eD32SfloatS8Uint;
        case DataFormat::Undefined:
        default:
            return vk::Format::eUndefined;
        }
    }

    DataFormat DataFormatFromVkFormat(vk::Format vk_format)
    {
		 switch (vk_format)
        {
        case vk::Format::eR8Unorm:
            return DataFormat::R8_UNorm;
        case vk::Format::eR8Snorm:
            return DataFormat::R8_SNorm;
        case vk::Format::eR8Uscaled:
            return DataFormat::R8_UScaled;
        case vk::Format::eR8Sscaled:
            return DataFormat::R8_SScaled;
        case vk::Format::eR8Uint:
            return DataFormat::R8_UInt;
        case vk::Format::eR8Sint:
            return DataFormat::R8_SInt;
        case vk::Format::eR16Unorm:
            return DataFormat::R16_UNorm;
        case vk::Format::eR16Snorm:
            return DataFormat::R16_SNorm;
        case vk::Format::eR16Uscaled:
            return DataFormat::R16_UScaled;
        case vk::Format::eR16Sscaled:
            return DataFormat::R16_SScaled;
        case vk::Format::eR16Uint:
            return DataFormat::R16_UInt;
        case vk::Format::eR16Sint:
            return DataFormat::R16_SInt;
        case vk::Format::eR16Sfloat:
            return DataFormat::R16_Float;
        case vk::Format::eR8G8Unorm:
            return DataFormat::R8G8_UNorm;
        case vk::Format::eR8G8Snorm:
            return DataFormat::R8G8_SNorm;
        case vk::Format::eR8G8Uscaled:
            return DataFormat::R8G8_UScaled;
        case vk::Format::eR8G8Sscaled:
            return DataFormat::R8G8_SScaled;
        case vk::Format::eR8G8Uint:
            return DataFormat::R8G8_UInt;
        case vk::Format::eR8G8Sint:
            return DataFormat::R8G8_SInt;
        case vk::Format::eR8G8Srgb:
            return DataFormat::R8G8_SRGB;
        case vk::Format::eR8G8B8Unorm:
            return DataFormat::R8G8B8_UNorm;
        case vk::Format::eR8G8B8Snorm:
            return DataFormat::R8G8B8_SNorm;
        case vk::Format::eR8G8B8Uscaled:
            return DataFormat::R8G8B8_UScaled;
        case vk::Format::eR8G8B8Sscaled:
            return DataFormat::R8G8B8_SScaled;
        case vk::Format::eR8G8B8Uint:
            return DataFormat::R8G8B8_UInt;
        case vk::Format::eR8G8B8Sint:
            return DataFormat::R8G8B8_SInt;
        case vk::Format::eR8G8B8Srgb:
            return DataFormat::R8G8B8_SRGB;
        case vk::Format::eB8G8R8Unorm:
            return DataFormat::B8G8R8_UNorm;
        case vk::Format::eB8G8R8Snorm:
            return DataFormat::B8G8R8_SNorm;
        case vk::Format::eB8G8R8Uscaled:
            return DataFormat::B8G8R8_UScaled;
        case vk::Format::eB8G8R8Sscaled:
            return DataFormat::B8G8R8_SScaled;
        case vk::Format::eB8G8R8Uint:
            return DataFormat::B8G8R8_UInt;
        case vk::Format::eB8G8R8Sint:
            return DataFormat::B8G8R8_SInt;
        case vk::Format::eB8G8R8Srgb:
            return DataFormat::B8G8R8_SRGB;
        case vk::Format::eR32Uint:
            return DataFormat::R32_UInt;
        case vk::Format::eR32Sint:
            return DataFormat::R32_SInt;
        case vk::Format::eR32Sfloat:
            return DataFormat::R32_Float;
        case vk::Format::eR16G16Unorm:
            return DataFormat::R16G16_UNorm;
        case vk::Format::eR16G16Snorm:
            return DataFormat::R16G16_SNorm;
        case vk::Format::eR16G16Uscaled:
            return DataFormat::R16G16_UScaled;
        case vk::Format::eR16G16Sscaled:
            return DataFormat::R16G16_SScaled;
        case vk::Format::eR16G16Uint:
            return DataFormat::R16G16_UInt;
        case vk::Format::eR16G16Sint:
            return DataFormat::R16G16_SInt;
        case vk::Format::eR16G16Sfloat:
            return DataFormat::R16G16_Float;
        case vk::Format::eR8G8B8A8Unorm:
            return DataFormat::R8G8B8A8_UNorm;
        case vk::Format::eR8G8B8A8Snorm:
            return DataFormat::R8G8B8A8_SNorm;
        case vk::Format::eR8G8B8A8Uscaled:
            return DataFormat::R8G8B8A8_UScaled;
        case vk::Format::eR8G8B8A8Sscaled:
            return DataFormat::R8G8B8A8_SScaled;
        case vk::Format::eR8G8B8A8Uint:
            return DataFormat::R8G8B8A8_UInt;
        case vk::Format::eR8G8B8A8Sint:
            return DataFormat::R8G8B8A8_SInt;
        case vk::Format::eR8G8B8A8Srgb:
            return DataFormat::R8G8B8A8_SRGB;
        case vk::Format::eB8G8R8A8Unorm:
            return DataFormat::B8G8R8A8_UNorm;
        case vk::Format::eB8G8R8A8Snorm:
            return DataFormat::B8G8R8A8_SNorm;
        case vk::Format::eB8G8R8A8Uscaled:
            return DataFormat::B8G8R8A8_UScaled;
        case vk::Format::eB8G8R8A8Sscaled:
            return DataFormat::B8G8R8A8_SScaled;
        case vk::Format::eB8G8R8A8Uint:
            return DataFormat::B8G8R8A8_UInt;
        case vk::Format::eB8G8R8A8Sint:
            return DataFormat::B8G8R8A8_SInt;
        case vk::Format::eB8G8R8A8Srgb:
            return DataFormat::B8G8R8A8_SRGB;
        case vk::Format::eR16G16B16Unorm:
            return DataFormat::R16G16B16_UNorm;
        case vk::Format::eR16G16B16Snorm:
            return DataFormat::R16G16B16_SNorm;
        case vk::Format::eR16G16B16Uscaled:
            return DataFormat::R16G16B16_UScaled;
        case vk::Format::eR16G16B16Sscaled:
            return DataFormat::R16G16B16_SScaled;
        case vk::Format::eR16G16B16Uint:
            return DataFormat::R16G16B16_UInt;
        case vk::Format::eR16G16B16Sint:
            return DataFormat::R16G16B16_SInt;
        case vk::Format::eR16G16B16Sfloat:
            return DataFormat::R16G16B16_Float;
        case vk::Format::eR64Uint:
            return DataFormat::R64_UInt;
        case vk::Format::eR64Sint:
            return DataFormat::R64_SInt;
        case vk::Format::eR64Sfloat:
            return DataFormat::R64_Float;
        case vk::Format::eR32G32Uint:
            return DataFormat::R32G32_UInt;
        case vk::Format::eR32G32Sint:
            return DataFormat::R32G32_SInt;
        case vk::Format::eR32G32Sfloat:
            return DataFormat::R32G32_Float;
        case vk::Format::eR16G16B16A16Unorm:
            return DataFormat::R16G16B16A16_UNorm;
        case vk::Format::eR16G16B16A16Snorm:
            return DataFormat::R16G16B16A16_SNorm;
        case vk::Format::eR16G16B16A16Uscaled:
            return DataFormat::R16G16B16A16_UScaled;
        case vk::Format::eR16G16B16A16Sscaled:
            return DataFormat::R16G16B16A16_SScaled;
        case vk::Format::eR16G16B16A16Uint:
            return DataFormat::R16G16B16A16_UInt;
        case vk::Format::eR16G16B16A16Sint:
            return DataFormat::R16G16B16A16_SInt;
        case vk::Format::eR16G16B16A16Sfloat:
            return DataFormat::R16G16B16A16_Float;
        case vk::Format::eR32G32B32Uint:
            return DataFormat::R32G32B32_UInt;
        case vk::Format::eR32G32B32Sint:
            return DataFormat::R32G32B32_SInt;
        case vk::Format::eR32G32B32Sfloat:
            return DataFormat::R32G32B32_Float;
        case vk::Format::eR64G64Uint:
            return DataFormat::R64G64_UInt;
        case vk::Format::eR64G64Sint:
            return DataFormat::R64G64_SInt;
        case vk::Format::eR64G64Sfloat:
            return DataFormat::R64G64_Float;
        case vk::Format::eR32G32B32A32Uint:
            return DataFormat::R32G32B32A32_UInt;
        case vk::Format::eR32G32B32A32Sint:
            return DataFormat::R32G32B32A32_SInt;
        case vk::Format::eR32G32B32A32Sfloat:
            return DataFormat::R32G32B32A32_Float;
        case vk::Format::eR64G64B64Uint:
            return DataFormat::R64G64B64_UInt;
        case vk::Format::eR64G64B64Sint:
            return DataFormat::R64G64B64_SInt;
        case vk::Format::eR64G64B64Sfloat:
            return DataFormat::R64G64B64_Float;
        case vk::Format::eR64G64B64A64Uint:
            return DataFormat::R64G64B64A64_UInt;
        case vk::Format::eR64G64B64A64Sint:
            return DataFormat::R64G64B64A64_SInt;
        case vk::Format::eR64G64B64A64Sfloat:
            return DataFormat::R64G64B64A64_Float;
        case vk::Format::eD16Unorm:
            return DataFormat::D16_UNorm;
        case vk::Format::eD32Sfloat:
            return DataFormat::D32_Float;
        case vk::Format::eS8Uint:
            return DataFormat::S8_UInt;
        case vk::Format::eD16UnormS8Uint:
            return DataFormat::D16_UNorm_S8_UInt;
        case vk::Format::eD24UnormS8Uint:
            return DataFormat::D24_UNorm_S8_UInt;
        case vk::Format::eD32SfloatS8Uint:
            return DataFormat::D32_Float_S8_UInt;
        default:
            return DataFormat::Undefined;
        }
    }

    bool IsDepthStencilDataFormat(DataFormat data_format)
    {
        if (data_format == DataFormat::D16_UNorm
         || data_format == DataFormat::D32_Float
         || data_format == DataFormat::S8_UInt
         || data_format == DataFormat::D16_UNorm_S8_UInt
         || data_format == DataFormat::D24_UNorm_S8_UInt
         || data_format == DataFormat::D32_Float_S8_UInt)
        {
            return true;
        }
        return false;
    }
}
