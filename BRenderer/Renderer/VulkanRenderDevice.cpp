#include "Renderer/VulkanRenderDevice.h"

#include <Renderer/RenderDefs.h>

#include <Visualization/Window.h>
#include <Core/LogSystem.h>
#include <Files/FilesUtils.h>
#include <Geometry/Geometry.h>

#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace brr::render
{
	std::unique_ptr<VulkanRenderDevice> VulkanRenderDevice::device_ {};

	void VulkanRenderDevice::CreateRenderDevice(vis::Window* window)
	{
		assert(!device_ && "VulkanRenderDevice is already created. You can only create one.");
		device_.reset(new VulkanRenderDevice(window));
	}

    VulkanRenderDevice* VulkanRenderDevice::GetSingleton()
	{
		assert(device_ && "Can't get non-initialized VulkanRenderDevice. Run `VKRD::CreateRenderDevice(Window* window)` before this function.");
	    return device_.get();
	}

    VulkanRenderDevice::~VulkanRenderDevice()
    {
		WaitIdle();

		vmaDestroyAllocator(m_vma_allocator);

		m_pDescriptorLayoutCache.reset();
		m_pDescriptorAllocator.reset();

		if (graphics_command_pool_)
		{
			m_device.destroyCommandPool(graphics_command_pool_);
			graphics_command_pool_ = VK_NULL_HANDLE;
		}
		if (present_command_pool_)
		{
			m_device.destroyCommandPool(present_command_pool_);
			present_command_pool_ = VK_NULL_HANDLE;
		}
		if (transfer_command_pool_)
		{
			m_device.destroyCommandPool(transfer_command_pool_);
			transfer_command_pool_ = VK_NULL_HANDLE;
		}

		m_device.destroy();
		std::cout << "Vulkan Renderer Destroyed";
    }

    VulkanRenderDevice::VulkanRenderDevice(vis::Window* main_window)
	{
		Init_VkInstance(main_window);
		vk::SurfaceKHR surface = main_window->GetVulkanSurface(vulkan_instance_);
		Init_PhysDevice(surface);
		Init_Queues_Indices(surface);
		Init_Device();
		Init_Allocator();
		Init_CommandPool();

		m_pDescriptorLayoutCache.reset(new DescriptorLayoutCache(m_device));
		m_pDescriptorAllocator.reset(new DescriptorAllocator(m_device));
	}

	vk::ShaderModule Create_ShaderModule(VulkanRenderDevice* device, std::vector<char>& code)
	{
		vk::ShaderModuleCreateInfo shader_module_info{};
		shader_module_info
			.setCodeSize(code.size())
			.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

		auto createShaderModuleResult = device->Get_VkDevice().createShaderModule(shader_module_info);
		if (createShaderModuleResult.result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not create ShaderModule! Result code: {}.", vk::to_string(createShaderModuleResult.result).c_str());
			exit(1);
		}
		return createShaderModuleResult.value;
	}

    Shader VulkanRenderDevice::CreateShaderFromFiles(std::string vertex_file_name, std::string frag_file_name)
    {
		Shader shader{};
		vk::ShaderModule vertex_shader_module;
		vk::ShaderModule fragment_shader_module;
		// Load Vertex Shader
		{
			std::filesystem::path file_path{ vertex_file_name + ".spv" };
			if (!file_path.has_filename())
			{
				BRR_LogInfo("'{}' is not a valid file path.", file_path.string());
				return Shader{};
			}

			std::vector<char> vertex_shader_code = files::ReadFile(file_path.string());

			vertex_shader_module = Create_ShaderModule(this, vertex_shader_code);

			shader.pipeline_stage_infos_.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(vertex_shader_module)
				.setPName("main"));
		}

		// Load Fragment Shader
		{
			std::filesystem::path file_path{ frag_file_name + ".spv" };
			if (!file_path.has_filename())
			{
				BRR_LogError("'{}' is not a valid file path.", file_path.string());
				return Shader{};
			}

			std::vector<char> fragment_shader_code = files::ReadFile(file_path.string());

			fragment_shader_module = Create_ShaderModule(this, fragment_shader_code);

			shader.pipeline_stage_infos_.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(fragment_shader_module)
				.setPName("main"));
		}

		shader.m_isValid = true;
		shader.m_pDevice = this;
		shader.vert_shader_module_ = vertex_shader_module;
		shader.frag_shader_module_ = fragment_shader_module;

		shader.vertex_input_binding_description_ = Vertex3_PosColor::GetBindingDescription();
		shader.vertex_input_attribute_descriptions_ = Vertex3_PosColor::GetAttributeDescriptions();

		return std::move(shader);
    }

    void VulkanRenderDevice::WaitIdle() const
    {
		m_device.waitIdle();
	}

    static vk::Result allocateCommandBuffer(vk::Device device, vk::CommandPool cmd_pool, vk::CommandBufferLevel level,
                                            uint32_t cmd_buffer_count, vk::CommandBuffer* out_command_buffers)
    {
        vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
        command_buffer_alloc_info
            .setCommandPool(cmd_pool)
            .setLevel(vk::CommandBufferLevel(level))
            .setCommandBufferCount(cmd_buffer_count);

		auto allocCmdBufferResult = device.allocateCommandBuffers(command_buffer_alloc_info);
		if (allocCmdBufferResult.result == vk::Result::eSuccess)
		{
			std::memcpy(out_command_buffers, allocCmdBufferResult.value.data(), cmd_buffer_count * sizeof(vk::CommandBuffer));
		}
		return allocCmdBufferResult.result;
	}

    vk::Result VulkanRenderDevice::AllocateGraphicsCommandBuffers(CommandBufferLevel level, uint32_t cmd_buffer_count,
                                                                 vk::CommandBuffer* out_command_buffers) const
    {
        return allocateCommandBuffer(m_device, graphics_command_pool_, vk::CommandBufferLevel(level), cmd_buffer_count,
                                     out_command_buffers);
    }

    vk::Result VulkanRenderDevice::AllocatePresentCommandBuffers(CommandBufferLevel level, uint32_t cmd_buffer_count,
                                                                vk::CommandBuffer* out_command_buffers) const
    {
        return allocateCommandBuffer(m_device, present_command_pool_, vk::CommandBufferLevel(level), cmd_buffer_count,
                                     out_command_buffers);
    }

    vk::Result VulkanRenderDevice::AllocateTransferCommandBuffers(CommandBufferLevel level, uint32_t cmd_buffer_count,
                                                                 vk::CommandBuffer* out_command_buffers) const
    {
        return allocateCommandBuffer(m_device, transfer_command_pool_, vk::CommandBufferLevel(level), cmd_buffer_count,
                                     out_command_buffers);
    }

    vk::Result VulkanRenderDevice::SubmitGraphicsCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                                uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
                                                                vk::PipelineStageFlags* wait_dst_stages,
                                                                uint32_t signal_semaphore_count, vk::Semaphore* signal_semaphores,
                                                                vk::Fence submit_fence)
    {
		vk::SubmitInfo submit_info{};
		submit_info
			.setPCommandBuffers(cmd_buffers)
			.setCommandBufferCount(cmd_buffer_count)
			.setPWaitSemaphores(wait_semaphores)
		    .setWaitSemaphoreCount(wait_semaphore_count)
			.setPWaitDstStageMask(wait_dst_stages)
			.setPSignalSemaphores(signal_semaphores)
	        .setSignalSemaphoreCount(signal_semaphore_count);

		/*vk::CommandBufferSubmitInfo cmd_buffer_submit_info {cmd_buffer};

		vk::SemaphoreSubmitInfo image_available_semaphore_info {};
		image_available_semaphore_info
			.setDeviceIndex(0)
			.setSemaphore(m_current_image_available_semaphore)
			.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

		vk::SemaphoreSubmitInfo render_finished_semaphore_info {};
		render_finished_semaphore_info
			.setDeviceIndex(0)
			.setSemaphore(current_render_finished_semaphore)
			.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

		vk::SubmitInfo2 submit_info2 {};
		submit_info2
			.setCommandBufferInfos(cmd_buffer_submit_info)
			.setWaitSemaphoreInfos(image_available_semaphore_info)
			.setSignalSemaphoreInfos(render_finished_semaphore_info);*/

		return GetGraphicsQueue().submit(submit_info, submit_fence);
    }

    vk::Result VulkanRenderDevice::SubmitPresentCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                               uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
		                                                       vk::PipelineStageFlags* wait_dst_stages,
                                                               uint32_t signal_semaphore_count, vk::Semaphore* signal_semaphores,
                                                               vk::Fence submit_fence)
    {
		//TODO
		return vk::Result::eErrorUnknown;
    }

    vk::Result VulkanRenderDevice::SubmitTransferCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                                uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
		                                                        vk::PipelineStageFlags* wait_dst_stages,
                                                                uint32_t signal_semaphore_count, vk::Semaphore* signal_semaphores,
                                                                vk::Fence submit_fence)
    {
		vk::SubmitInfo submit_info{};
		submit_info
			.setPCommandBuffers(cmd_buffers)
			.setCommandBufferCount(cmd_buffer_count)
			.setPWaitSemaphores(wait_semaphores)
			.setWaitSemaphoreCount(wait_semaphore_count)
			.setPWaitDstStageMask(wait_dst_stages)
			.setPSignalSemaphores(signal_semaphores)
			.setSignalSemaphoreCount(signal_semaphore_count);

		return GetTransferQueue().submit(submit_info, submit_fence);
    }

    DescriptorLayoutBuilder VulkanRenderDevice::GetDescriptorLayoutBuilder() const
    {
		return DescriptorLayoutBuilder::MakeDescriptorLayoutBuilder(m_pDescriptorLayoutCache.get());
    }

    DescriptorSetBuilder<FRAME_LAG> VulkanRenderDevice::GetDescriptorSetBuilder(
        const DescriptorLayout& layout) const
    {
		return DescriptorSetBuilder<FRAME_LAG>::MakeDescriptorSetBuilder(layout, m_pDescriptorAllocator.get());
    }

    void VulkanRenderDevice::Create_Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags buffer_usage,
                                           VmaMemoryUsage memory_usage, vk::Buffer& buffer,
                                           VmaAllocation& buffer_allocation, 
		                                   VmaAllocationCreateFlags buffer_allocation_flags)
    {
		// Create Buffer
		{
			vk::SharingMode sharing_mode = IsDifferentTransferQueue() ? vk::SharingMode::eExclusive : vk::SharingMode::eExclusive;
			std::vector<uint32_t> indices{
				GetQueueFamilyIndices().m_graphicsFamily.value(),
				GetQueueFamilyIndices().m_transferFamily.value()
			};

			vk::BufferCreateInfo buffer_create_info;
			buffer_create_info
				.setUsage(buffer_usage)
				.setSharingMode(sharing_mode)
				.setSize(buffer_size);
			if (sharing_mode == vk::SharingMode::eConcurrent)
			{

				buffer_create_info.setQueueFamilyIndices(indices);
			}

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = memory_usage;
			allocInfo.flags = buffer_allocation_flags;
			allocInfo.requiredFlags = 0;
			allocInfo.preferredFlags = 0;
			allocInfo.memoryTypeBits = 0;
			allocInfo.pool = VK_NULL_HANDLE;
			allocInfo.pUserData = nullptr;
			allocInfo.priority = 1.0;

			VkBuffer new_buffer;
			VmaAllocation allocation;
			VmaAllocationInfo allocation_info;
            const vk::Result createBufferResult = vk::Result(vmaCreateBuffer(m_vma_allocator, reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &allocInfo,
                                                                             &new_buffer, &allocation, &allocation_info));

			if (createBufferResult != vk::Result::eSuccess)
			{
				BRR_LogError("Could not create Buffer! Result code: {}.", vk::to_string(createBufferResult).c_str());
				exit(1);
			}
			buffer = new_buffer;
			buffer_allocation = allocation;

			BRR_LogInfo("Buffer created.");
		}

		return;
    }

    void VulkanRenderDevice::Copy_Buffer_Immediate(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size,
        vk::DeviceSize src_buffer_offset, vk::DeviceSize dst_buffer_offset)
    {
		const vk::CommandPool transfer_cmd_pool = (IsDifferentTransferQueue()) ? transfer_command_pool_ : graphics_command_pool_;

		vk::CommandBufferAllocateInfo cmd_buffer_alloc_info{};
		cmd_buffer_alloc_info
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandPool(transfer_cmd_pool)
			.setCommandBufferCount(1);

		auto allocCmdBuffersResult = m_device.allocateCommandBuffers(cmd_buffer_alloc_info);
		if (allocCmdBuffersResult.result != vk::Result::eSuccess)
		{
			BRR_LogError("ERROR: Could not allocate CommandBuffer! Result code: {}.", vk::to_string(allocCmdBuffersResult.result).c_str());
			exit(1);
		}
		vk::CommandBuffer cmd_buffer = allocCmdBuffersResult.value[0];

		vk::CommandBufferBeginInfo cmd_begin_info{};
		cmd_begin_info
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		cmd_buffer.begin(cmd_begin_info);

		vk::BufferCopy copy_region{};
		copy_region
			.setSrcOffset(src_buffer_offset)
			.setDstOffset(dst_buffer_offset)
			.setSize(size);

		cmd_buffer.copyBuffer(src_buffer, dst_buffer, copy_region);

		cmd_buffer.end();

		// For now, waiting the device to finish everything before copying data to a potentially used buffer.
		// TODO: Do correct synchronization (add copies to a setup command buffer)
		WaitIdle();

		SubmitTransferCommandBuffers(1, &cmd_buffer, 0, nullptr, nullptr, 0, nullptr, VK_NULL_HANDLE);

		GetTransferQueue().waitIdle();

		m_device.freeCommandBuffers(transfer_cmd_pool, cmd_buffer);
    }

	void VulkanRenderDevice::Init_VkInstance(vis::Window* window)
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

			{
				LogStreamBuffer log_msg = BRR_InfoStrBuff();
				log_msg << "Available Layers:";
				for (vk::LayerProperties& layer : layers)
				{
					log_msg << "\n\tLayer name: " << layer.layerName;
				}
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

		    {
		        LogStreamBuffer log_msg = BRR_InfoStrBuff();
				log_msg << "Available Instance Extensions:";
			    for (vk::ExtensionProperties& extension : extension_properties)
			    {
			        log_msg << "\n\tExtension name: " << extension.extensionName;
			    }
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

	void VulkanRenderDevice::Init_PhysDevice(vk::SurfaceKHR surface)
	{
		auto enumPhysDevicesResult = vulkan_instance_.enumeratePhysicalDevices();
		std::vector<vk::PhysicalDevice> devices = enumPhysDevicesResult.value;

		if (devices.size() == 0)
		{
			BRR_LogError("Failed to find a physical device with Vulkan support. Exitting program.");
			exit(1);
		}

	    {
			LogStreamBuffer log_msg = BRR_InfoStrBuff();
			log_msg << "Available devices:";
		    for (vk::PhysicalDevice& device : devices)
		    {
		        log_msg << "\n\tDevice: " << device.getProperties().deviceName;
		    }
	    }

		phys_device_ = VkHelpers::Select_PhysDevice(devices, surface);
		std::string device_name = phys_device_.getProperties().deviceName;

		auto device_extensions = phys_device_.enumerateDeviceExtensionProperties();
		if (device_extensions.result == vk::Result::eSuccess)
		{
			LogStreamBuffer log_msg = BRR_InfoStrBuff();
			log_msg << "Available Device Extensions (" << device_name << "):";
		    for (vk::ExtensionProperties& extension : device_extensions.value)
		    {
				log_msg << "\n\tExtension name: " << extension.extensionName;
		    }
		}

		BRR_LogInfo("Selected physical device: {}", phys_device_.getProperties().deviceName);
	}

	void VulkanRenderDevice::Init_Queues_Indices(vk::SurfaceKHR surface)
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

	void VulkanRenderDevice::Init_Device()
	{
		if (!queue_family_indices_.m_graphicsFamily.has_value())
		{
			BRR_LogError("Cannot create device without initializing at least the graphics queue.");
			exit(1);
		}

		const uint32_t graphics_family_idx = queue_family_indices_.m_graphicsFamily.value();
		const uint32_t presentation_family_idx = queue_family_indices_.m_presentFamily.value();
		const uint32_t transfer_family_idx = queue_family_indices_.m_transferFamily.has_value() ? queue_family_indices_.m_transferFamily.value() : graphics_family_idx;

		BRR_LogInfo("Selected Queue Families:\n"
			        "\tGraphics Queue Family:\t {}\n"
		            "\tPresent Queue Family:\t {}\n"
			        "\tTransfer Queue Family:\t {}", 
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

		vk::PhysicalDeviceSynchronization2Features synchronization2_features {};
		synchronization2_features.setSynchronization2(true);

		vk::DeviceCreateInfo device_create_info = vk::DeviceCreateInfo{};
		device_create_info
		    .setPNext(&synchronization2_features)
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
		m_device = createDeviceResult.value;
	    {
	        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
			BRR_LogInfo("Loaded Device specific Vulkan functions addresses.");
	    }

		graphics_queue_ = m_device.getQueue(graphics_family_idx, 0);

		presentation_queue_ = (different_present_queue_) ? m_device.getQueue(presentation_family_idx, 0) : graphics_queue_;

		transfer_queue_ = (different_transfer_queue_) ? m_device.getQueue(transfer_family_idx, 0) : graphics_queue_;

		BRR_LogInfo("Device Created");
	}

	void VulkanRenderDevice::Init_Allocator()
	{
		VmaVulkanFunctions vulkan_functions{};
		// Initialize function pointers
		{
			vulkan_functions.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
			vulkan_functions.vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;
			vulkan_functions.vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties;
			vulkan_functions.vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties;
			vulkan_functions.vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory;
			vulkan_functions.vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory;
			vulkan_functions.vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory;
			vulkan_functions.vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory;
			vulkan_functions.vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges;
			vulkan_functions.vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges;
			vulkan_functions.vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory;
			vulkan_functions.vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory;
			vulkan_functions.vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements;
			vulkan_functions.vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements;
			vulkan_functions.vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer;
			vulkan_functions.vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer;
			vulkan_functions.vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage;
			vulkan_functions.vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage;
			vulkan_functions.vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer;
			vulkan_functions.vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2;
			vulkan_functions.vkGetImageMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2;
			vulkan_functions.vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2;
			vulkan_functions.vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2;
			vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2;
			vulkan_functions.vkGetDeviceBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceBufferMemoryRequirements;
			vulkan_functions.vkGetDeviceImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceImageMemoryRequirements;
		}

		VmaAllocatorCreateInfo vma_alloc_create_info {};
		vma_alloc_create_info.device = m_device;
		vma_alloc_create_info.instance = vulkan_instance_;
		vma_alloc_create_info.physicalDevice = phys_device_;
		vma_alloc_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
		vma_alloc_create_info.pVulkanFunctions = &vulkan_functions;

		vmaCreateAllocator(&vma_alloc_create_info, &m_vma_allocator);
	}

	void VulkanRenderDevice::Init_CommandPool()
	{
		vk::CommandPoolCreateInfo command_pool_info{};
		command_pool_info
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(queue_family_indices_.m_graphicsFamily.value());

		 auto createCmdPoolResult = m_device.createCommandPool(command_pool_info);
		 if (createCmdPoolResult.result != vk::Result::eSuccess)
		 {
			 BRR_LogError("Could not create CommandPool! Result code: {}.", vk::to_string(createCmdPoolResult.result).c_str());
			 exit(1);
		 }
		 graphics_command_pool_ = createCmdPoolResult.value;

		 BRR_LogInfo("CommandPool created.");

		if (different_present_queue_)
		{
			vk::CommandPoolCreateInfo present_pool_info{};
			present_pool_info
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(queue_family_indices_.m_presentFamily.value());

			 auto createPresentCmdPoolResult = m_device.createCommandPool(present_pool_info);
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

			 auto createTransferCommandPoolResult = m_device.createCommandPool(transfer_pool_info);
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
