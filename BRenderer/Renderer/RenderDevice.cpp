#include "Renderer/RenderDevice.h"

#include "Core/Window.h"
#include "Core/LogSystem.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace brr::render
{

	RenderDevice::RenderDevice(Window* main_window)
	{
		Init_VkInstance(main_window);
		vk::SurfaceKHR surface = main_window->GetVulkanSurface(vulkan_instance_);
		Init_PhysDevice(surface);
		Init_Queues_Indices(surface);
		Init_Device();
		Init_CommandPool();
	}

	void RenderDevice::CreateShaderFromFilename(std::string file_name)
	{
		//Shader shader{};
		//// Load Vertex Shader
		//{
		//	std::filesystem::path file_path{ path + ".vert" };
		//	if (!file_path.has_filename())
		//	{
		//		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "'%s' is not a valid file path.");
		//		return Shader{};
		//	}

		//	std::vector<char> vertex_shader_code = files::ReadFile(path);

		//	vk::ShaderModule vertex_shader_module = Create_ShaderModule(vertex_shader_code);

		//	shader.m_pPipelineStageInfos.push_back(vk::PipelineShaderStageCreateInfo()
		//		.setStage(vk::ShaderStageFlagBits::eVertex)
		//		.setModule(vertex_shader_module)
		//		.setPName("main"));
		//}

		//// Load Fragment Shader
		//{
		//	std::filesystem::path file_path{ path + ".frag" };
		//	if (!file_path.has_filename())
		//	{
		//		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "'%s' is not a valid file path.");
		//		return Shader{};
		//	}

		//	std::vector<char> fragment_shader_code = files::ReadFile(path);

		//	vk::ShaderModule fragment_shader_module = Create_ShaderModule(fragment_shader_code);

		//	shader.m_pPipelineStageInfos.push_back(vk::PipelineShaderStageCreateInfo()
		//		.setStage(vk::ShaderStageFlagBits::eFragment)
		//		.setModule(fragment_shader_module)
		//		.setPName("main"));
		//}

		//shader.m_isValid = true;

		//return shader;
	}

	void RenderDevice::WaitIdle()
	{
		device_.waitIdle();
	}

	void RenderDevice::Init_VkInstance(Window* window)
	{
		// Dynamic load of the library
	    {
	        vk::DynamicLoader dl;
	        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
			if (vkGetInstanceProcAddr == nullptr)
			{
				BRR_LogError("Could not load 'vkGetInstanceProcAddr' function address. Can't create vkInstance.");
				exit(1);
			}
	        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
			BRR_LogInfo("Loaded 'vkGetInstanceProcAddr' function address. Proceeding to create vkInstance.");
	    }

		//TODO: Do validation layers
		// Check for validation layers support
#ifdef NDEBUG
		constexpr bool use_validation_layers = false;
#else
		constexpr bool use_validation_layers = true;
#endif

		std::vector<char const*> instance_validation_layers = { "VK_LAYER_KHRONOS_validation" };
		std::vector<char const*> enabled_validation_layers;
		if (use_validation_layers)
		{
			auto enumInstLayerPropsResult = vk::enumerateInstanceLayerProperties();
			std::vector<vk::LayerProperties> layers = enumInstLayerPropsResult.value;
			std::vector<char const*> acceptedLayers;
			if (VkHelpers::Check_ValidationLayers(instance_validation_layers, layers, acceptedLayers))
			{
				enabled_validation_layers = acceptedLayers;
			}
			else
			{
				BRR_LogError("Could not find any of the required validation layers.");
				/*exit(1);*/
			}
		}

		// Gather required extensions
		std::vector<const char*> extensions{};
		{
			window->GetRequiredVulkanExtensions(extensions);

			// TODO: Check if the required extensions are supported by Vulkan
			uint32_t extension_count = 0;
			auto enumInstExtPropsResult = vk::enumerateInstanceExtensionProperties();
			std::vector<vk::ExtensionProperties> extension_properties = enumInstExtPropsResult.value;

			BRR_LogInfo("Available Extensions");
			for (vk::ExtensionProperties& extension : extension_properties)
			{
				BRR_LogInfo("\tExtension name: {}", extension.extensionName);

			}
		}

		vk::ApplicationInfo app_info{ };
		app_info.setPApplicationName("Brayner Renderer");
		app_info.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
		app_info.setApiVersion(VK_API_VERSION_1_3);

		vk::InstanceCreateInfo inst_create_info{};
		inst_create_info
			.setPApplicationInfo(&app_info)
			.setPEnabledExtensionNames(extensions)
			.setPEnabledLayerNames(enabled_validation_layers);

		 auto createInstanceResult = vk::createInstance(inst_create_info);
		 if (createInstanceResult.result != vk::Result::eSuccess)
		 {
			 BRR_LogError("Could not create Vulkan Instance! Result code: {}.", vk::to_string(createInstanceResult.result).c_str());
			 exit(1);
		 }
		 vulkan_instance_ = createInstanceResult.value;

	    {
	        VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkan_instance_);
			BRR_LogInfo("Loaded instance specific vulkan functions addresses.");
	    }

		 BRR_LogInfo("Instance Created");
	}

	void RenderDevice::Init_PhysDevice(vk::SurfaceKHR surface)
	{
		auto enumPhysDevicesResult = vulkan_instance_.enumeratePhysicalDevices();
		std::vector<vk::PhysicalDevice> devices = enumPhysDevicesResult.value;

		if (devices.size() == 0)
		{
			BRR_LogError("Failed to find a physical device with Vulkan support. Exitting program.");
			exit(1);
		}

		BRR_LogInfo("Available devices:");
		for (vk::PhysicalDevice& device : devices)
		{
			BRR_LogInfo("\tDevice: {}", device.getProperties().deviceName);
		}

		phys_device_ = VkHelpers::Select_PhysDevice(devices, surface);

		BRR_LogInfo("Selected physical device: {}", phys_device_.getProperties().deviceName);
	}

	void RenderDevice::Init_Queues_Indices(vk::SurfaceKHR surface)
	{
		// Check for queue families
		queue_family_indices_ = VkHelpers::Find_QueueFamilies(phys_device_, surface);

		if (!queue_family_indices_.m_graphicsFamily.has_value())
		{
			BRR_LogError("Failed to find graphics family queue. Exitting program.");
			exit(1);
		}

		if (!queue_family_indices_.m_presentFamily.has_value())
		{
			BRR_LogError("Failed to find presentation family queue. Exitting program.");
			exit(1);
		}
	}

	void RenderDevice::Init_Device()
	{
		if (!queue_family_indices_.m_graphicsFamily.has_value())
		{
			BRR_LogError("Cannot create device without initializing at least the graphics queue.");
			exit(1);
		}

		const uint32_t graphics_family_idx = queue_family_indices_.m_graphicsFamily.value();
		const uint32_t presentation_family_idx = queue_family_indices_.m_presentFamily.value();
		const uint32_t transfer_family_idx = queue_family_indices_.m_transferFamily.has_value() ? queue_family_indices_.m_transferFamily.value() : graphics_family_idx;

		BRR_LogInfo("Graphics Queue Family:\t {}\n"
		            "Present Queue Family:\t {}\n"
			        "Transfer Queue Family:\t {}", 
			        graphics_family_idx, 
			        presentation_family_idx, 
			        transfer_family_idx);

		float priorities = 1.0;
		std::vector<vk::DeviceQueueCreateInfo> queues;

		queues.push_back(vk::DeviceQueueCreateInfo{}
			.setQueueFamilyIndex(graphics_family_idx)
			.setQueuePriorities(priorities));

		different_present_queue_ = graphics_family_idx != presentation_family_idx;
		if (different_present_queue_)
		{
			queues.push_back(vk::DeviceQueueCreateInfo{}
				.setQueueFamilyIndex(presentation_family_idx)
				.setQueuePriorities(priorities));
		}

		different_transfer_queue_ = graphics_family_idx != transfer_family_idx;
		if (different_transfer_queue_)
		{
			queues.push_back(vk::DeviceQueueCreateInfo{}
				.setQueueFamilyIndex(transfer_family_idx)
				.setQueuePriorities(priorities));
		}

		vk::PhysicalDeviceFeatures device_features{};

		std::vector<const char*> device_extensions{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		vk::DeviceCreateInfo device_create_info = vk::DeviceCreateInfo{};
		device_create_info
			.setQueueCreateInfos(queues)
			.setPEnabledFeatures(&device_features)
			.setEnabledLayerCount(0)
			.setPEnabledExtensionNames(device_extensions);

        auto createDeviceResult = phys_device_.createDevice(device_create_info);
        if (createDeviceResult.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create Vulkan Device! Result code: {}.", vk::to_string(createDeviceResult.result).c_str());
            exit(1);
        }
        device_ = createDeviceResult.value;
	    {
	        VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);
			BRR_LogInfo("Loaded Device specific Vulkan functions addresses.");
	    }

		graphics_queue_ = device_.getQueue(graphics_family_idx, 0);

		presentation_queue_ = (different_present_queue_) ? device_.getQueue(presentation_family_idx, 0) : graphics_queue_;

		transfer_queue_ = (different_transfer_queue_) ? device_.getQueue(transfer_family_idx, 0) : graphics_queue_;

		BRR_LogInfo("Device Created");
	}

	void RenderDevice::Init_CommandPool()
	{
		vk::CommandPoolCreateInfo command_pool_info{};
		command_pool_info
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(queue_family_indices_.m_graphicsFamily.value());

		 auto createCmdPoolResult = device_.createCommandPool(command_pool_info);
		 if (createCmdPoolResult.result != vk::Result::eSuccess)
		 {
			 BRR_LogError("Could not create CommandPool! Result code: {}.", vk::to_string(createCmdPoolResult.result).c_str());
			 exit(1);
		 }
		 command_pool_ = createCmdPoolResult.value;

		 BRR_LogInfo("CommandPool created.");

		if (different_present_queue_)
		{
			vk::CommandPoolCreateInfo present_pool_info{};
			present_pool_info
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(queue_family_indices_.m_presentFamily.value());

			 auto createPresentCmdPoolResult = device_.createCommandPool(present_pool_info);
			 if (createPresentCmdPoolResult.result != vk::Result::eSuccess)
			 {
				 BRR_LogError("Could not create present CommandPool! Result code: {}.", vk::to_string(createPresentCmdPoolResult.result).c_str());
				 exit(1);
			 }
			 present_command_pool_ = createPresentCmdPoolResult.value;

			 BRR_LogInfo("Separate Present CommandPool created.");
		}

		if (different_transfer_queue_)
		{
			vk::CommandPoolCreateInfo transfer_pool_info{};
			transfer_pool_info
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(queue_family_indices_.m_transferFamily.value());

			 auto createTransferCommandPoolResult = device_.createCommandPool(transfer_pool_info);
			 if (createTransferCommandPoolResult.result != vk::Result::eSuccess)
			 {
				 BRR_LogError("Could not create transfer CommandPool! Result code: {}.", vk::to_string(createTransferCommandPoolResult.result).c_str());
				 exit(1);
			 }
			 transfer_command_pool_ = createTransferCommandPoolResult.value;

			 BRR_LogInfo("Separate Transfer CommandPool created.");
		}
	}
}
