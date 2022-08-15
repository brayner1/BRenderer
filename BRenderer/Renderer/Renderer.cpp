#include "Core/Window.h"
#include "Renderer/Renderer.h"

#include "Shader.h"

namespace brr::render
{
	std::unique_ptr<Renderer> Renderer::singleton = nullptr;

	Renderer::Renderer()
	{
		if (!singleton)
		{
			singleton.reset(this);
		}
	}

	Renderer::~Renderer()
	{
		Reset();
	}

	void Renderer::Init_VulkanRenderer(Window* main_window)
	{
		Init_VkInstance(main_window);
		Create_Window(main_window);
	}

	void Renderer::Window_Resized(Window* window)
	{
		uint32_t window_index = m_pWindowId_index_map[window->GetWindowID()];
		RendererWindow& rend_window = m_pWindows[window_index];

		Recreate_Swapchain(rend_window);
	}

	void Renderer::Create_Window(Window* window)
	{
		m_pWindows.resize(m_pWindow_number + 1);
		RendererWindow& rend_window = m_pWindows[m_pWindow_number];
		m_pWindowId_index_map[window->GetWindowID()] = m_pWindow_number;
		m_pWindow_number++;

		rend_window.m_associated_window = window;
		Init_Surface(rend_window);
		if (!m_pDevice)
		{
			Init_PhysDevice(rend_window.m_surface);
			if (!m_pQueues_initialized)
				Init_Queues(rend_window.m_surface);
			Init_Device();
		}
		
		Init_Swapchain(rend_window);
		Init_RenderPass(rend_window);
		Init_GraphicsPipeline(rend_window);
		Init_Framebuffers(rend_window);
		Init_CommandPool();
		Init_CommandBuffers(rend_window);
		Init_Sychronization(rend_window);
	}

	void Renderer::Destroy_Window(Window* window)
	{
		if (m_pDevice)
			m_pDevice.waitIdle();
		Reset();
	}

	void Renderer::Init_VkInstance(Window* window)
	{
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
			std::vector<vk::LayerProperties> layers = vk::enumerateInstanceLayerProperties();
			if (VkHelpers::Check_ValidationLayers(instance_validation_layers, layers))
			{
				enabled_validation_layers = instance_validation_layers;
			}
			else
			{
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not find required validation layers. Exiting application.");
				exit(1);
			}
		}

		// Gather required extensions
		std::vector<const char*> extensions{};
		{
			window->GetRequiredVulkanExtensions(extensions);

		// TODO: Check if the required extensions are supported by Vulkan
			uint32_t extension_count = 0;
			std::vector<vk::ExtensionProperties> extension_properties = vk::enumerateInstanceExtensionProperties();

			SDL_Log("Available Extensions");
			for (vk::ExtensionProperties& extension : extension_properties)
			{
				SDL_Log("\tExtension name: %s", extension.extensionName);

			}
		}

		vk::ApplicationInfo app_info{ };
		app_info.setPApplicationName("Brayn Renderer");
		app_info.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
		app_info.setApiVersion(VK_API_VERSION_1_3);

		vk::InstanceCreateInfo inst_create_info{};
		inst_create_info
			.setPApplicationInfo(&app_info)
			.setPEnabledExtensionNames(extensions)
			.setPEnabledLayerNames(enabled_validation_layers);

		m_pVkInstance = vk::createInstance(inst_create_info);

		SDL_Log("Instance Created");
	}

	void Renderer::Init_Surface(RendererWindow& window)
	{
		window.m_surface = window.m_associated_window->GetVulkanSurface(m_pVkInstance);

		if (!window.m_surface)
		{
			exit(1);
		}

		SDL_Log("Surface Created");
	}

	void Renderer::Init_Queues(vk::SurfaceKHR surface)
	{
		// Check for queue families
		m_pQueueFamilyIdx = VkHelpers::Find_QueueFamilies(m_pPhysDevice, surface);

		if (!m_pQueueFamilyIdx.m_graphicsFamily.has_value())
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find graphics family queue. Exitting program.");
			exit(1);
		}

		if (!m_pQueueFamilyIdx.m_presentFamily.has_value())
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find presentation family queue. Exitting program.");
			exit(1);
		}

		m_pQueues_initialized = true;
	}

	void Renderer::Init_PhysDevice(vk::SurfaceKHR surface)
	{
		std::vector<vk::PhysicalDevice> devices = m_pVkInstance.enumeratePhysicalDevices();

		if (devices.size() == 0)
		{
			SDL_Log("Failed to find a physical device with Vulkan support. Exitting program.");
			exit(1);
		}

		SDL_Log("Available devices:");
		for (vk::PhysicalDevice& device : devices)
		{
			SDL_Log("\tDevice: %s", device.getProperties().deviceName);
		}

		m_pPhysDevice = VkHelpers::Select_PhysDevice(devices, surface);

		SDL_Log("Selected physical device: %s", m_pPhysDevice.getProperties().deviceName);
	}

	void Renderer::Init_Device()
	{
		if (!m_pQueues_initialized)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot create device without initializing queues.");
			exit(1);
		}

		const uint32_t graphics_family_idx = m_pQueueFamilyIdx.m_graphicsFamily.value();
		const uint32_t presentation_family_idx = m_pQueueFamilyIdx.m_presentFamily.value();
		
		float priorities = 1.0;
		std::vector<vk::DeviceQueueCreateInfo> queues;

		queues.push_back(vk::DeviceQueueCreateInfo{}
			.setQueueFamilyIndex(graphics_family_idx)
			.setQueuePriorities(priorities));

		m_pDifferentPresentQueue = graphics_family_idx != presentation_family_idx;
		if (m_pDifferentPresentQueue)
		{
			queues.push_back(vk::DeviceQueueCreateInfo{}
				.setQueueFamilyIndex(presentation_family_idx)
				.setQueuePriorities(priorities));
		}

		vk::PhysicalDeviceFeatures device_features {};

		std::vector<const char*> device_extensions{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		vk::DeviceCreateInfo device_create_info = vk::DeviceCreateInfo{};
		device_create_info
			.setQueueCreateInfos(queues)
			.setPEnabledFeatures(&device_features)
			.setEnabledLayerCount(0)
			.setPEnabledExtensionNames(device_extensions);

		m_pDevice = m_pPhysDevice.createDevice(device_create_info);

		m_pGraphicsQueue = m_pDevice.getQueue(graphics_family_idx, 0);

		m_pPresentationQueue = (m_pDifferentPresentQueue)? m_pDevice.getQueue(presentation_family_idx, 0) : m_pGraphicsQueue;
		
		SDL_Log("Device Created");
	}

	void Renderer::Init_Swapchain(RendererWindow& window)
	{
		VkHelpers::SwapChainProperties properties = VkHelpers::Query_SwapchainProperties(m_pPhysDevice, window.m_surface);

		vk::SurfaceFormatKHR surface_format = VkHelpers::Select_SwapchainFormat(properties.m_surfFormats);
		vk::PresentModeKHR present_mode = VkHelpers::Select_SwapchainPresentMode(properties.m_presentModes);
		m_pSwapchainExtent = VkHelpers::Select_SwapchainExtent(window.m_associated_window, properties.m_surfCapabilities);
		m_pSwapchain_ImageFormat = surface_format.format;

		uint32_t imageCount = properties.m_surfCapabilities.minImageCount + 1;

		if (properties.m_surfCapabilities.maxImageCount > 0 && imageCount > properties.m_surfCapabilities.maxImageCount)
		{
			imageCount = properties.m_surfCapabilities.maxImageCount;
		}

		vk::SurfaceTransformFlagBitsKHR preTransform;
		if (properties.m_surfCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
			preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		}
		else {
			preTransform = properties.m_surfCapabilities.currentTransform;
		}

		vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		std::array<vk::CompositeAlphaFlagBitsKHR, 4> compositeAlphaFlags = {
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
			vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
			vk::CompositeAlphaFlagBitsKHR::eInherit,
		};
		for (const auto& compositeAlphaFlag : compositeAlphaFlags) {
			if (properties.m_surfCapabilities.supportedCompositeAlpha & compositeAlphaFlag) {
				compositeAlpha = compositeAlphaFlag;
				break;
			}
		}

		vk::SwapchainCreateInfoKHR swapchain_create_info {};
		swapchain_create_info
			.setSurface(window.m_surface)
			.setMinImageCount(imageCount)
			.setImageFormat(m_pSwapchain_ImageFormat)
			.setImageColorSpace(surface_format.colorSpace)
			.setImageExtent(m_pSwapchainExtent)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setPresentMode(present_mode)
			.setClipped(true)
			.setPreTransform(preTransform)
			.setCompositeAlpha(compositeAlpha)
			.setOldSwapchain(window.m_swapchain);

		{
			uint32_t graphics_family_queue = m_pQueueFamilyIdx.m_graphicsFamily.value();
			uint32_t presentation_family_queue = m_pQueueFamilyIdx.m_presentFamily.value();

			if (graphics_family_queue == presentation_family_queue)
			{
				swapchain_create_info.setImageSharingMode(vk::SharingMode::eExclusive);
			}
			else
			{
				std::vector<uint32_t> queue_family_indices { graphics_family_queue, presentation_family_queue };
				swapchain_create_info
					.setImageSharingMode(vk::SharingMode::eConcurrent)
					.setQueueFamilyIndices(queue_family_indices);
			}
		}

		vk::SwapchainKHR new_swapchain = m_pDevice.createSwapchainKHR(swapchain_create_info);
		SDL_Log("Swapchain created");

		// If old swapchain is valid, destroy it. (It happens on swapchain recreation)
		if (window.m_swapchain)
		{
			Cleanup_Swapchain(window);
		}

		// Assign the new swapchain to the window
		window.m_swapchain = new_swapchain;

		// Acquire swapchain images and create ImageViews
		{
			std::vector<vk::Image> swapchain_images = m_pDevice.getSwapchainImagesKHR(window.m_swapchain);
			window.m_image_resources.resize(swapchain_images.size());
			for (uint32_t i = 0; i < window.m_image_resources.size(); i++)
			{
				window.m_image_resources[i].m_image = swapchain_images[i];

				vk::ImageViewCreateInfo image_view_create_info {};
				image_view_create_info
					.setImage(swapchain_images[i])
					.setViewType(vk::ImageViewType::e2D)
					.setFormat(m_pSwapchain_ImageFormat)
					.setSubresourceRange(vk::ImageSubresourceRange{}
						.setAspectMask(vk::ImageAspectFlagBits::eColor)
						.setBaseMipLevel(0)
						.setLevelCount(1)
						.setLayerCount(1)
						.setBaseArrayLayer(0)
					);

				window.m_image_resources[i].m_image_view = m_pDevice.createImageView(image_view_create_info);
			}
		}

		SDL_Log("Images Resources initialized.");
	}

	void Renderer::Init_RenderPass(RendererWindow& window)
	{
		vk::AttachmentDescription color_attachment{};
		color_attachment
			.setFormat(m_pSwapchain_ImageFormat)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

		vk::AttachmentReference color_attachment_ref {};
		color_attachment_ref
			.setAttachment(0)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription subpass_description {};
		subpass_description
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachments(color_attachment_ref);

		vk::SubpassDependency subpass_dependency {};
		subpass_dependency
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlagBits::eNone)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead)
			.setDependencyFlags(vk::DependencyFlags());

		vk::RenderPassCreateInfo render_pass_info {};
		render_pass_info
			.setAttachments(color_attachment)
			.setSubpasses(subpass_description)
			.setDependencies(subpass_dependency);

		window.m_render_pass = m_pDevice.createRenderPass(render_pass_info);

		SDL_Log("Render Pass Created");
	}

	void Renderer::Init_GraphicsPipeline(RendererWindow& window)
	{
		Shader shader = Shader::Create_Shader("vert", "frag");

		vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info
			.setVertexBindingDescriptionCount(0)
			.setVertexAttributeDescriptionCount(0);
			

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false);

		vk::Viewport viewport {};
		viewport
			.setX(0.f)
			.setY(0.f)
			.setWidth((float) m_pSwapchainExtent.width)
			.setHeight((float) m_pSwapchainExtent.height)
			.setMinDepth(0.f)
			.setMaxDepth(1.f);

		vk::Rect2D scissor{ {0, 0}, m_pSwapchainExtent };

		vk::PipelineViewportStateCreateInfo viewport_state_info {};
		viewport_state_info
			.setViewportCount(1)
			.setViewports(viewport)
			.setScissorCount(1)
			.setScissors(scissor);

		vk::PipelineRasterizationStateCreateInfo rasterization_state_info {};
		rasterization_state_info
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1.f)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eClockwise)
			.setDepthBiasEnable(false)
			.setDepthBiasConstantFactor(0.f)
			.setDepthBiasClamp(0.f)
			.setDepthBiasSlopeFactor(0.f);

		vk::PipelineMultisampleStateCreateInfo multisampling_info {};
		multisampling_info
			.setSampleShadingEnable(false)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setMinSampleShading(1.f)
			.setPSampleMask(nullptr)
			.setAlphaToCoverageEnable(false)
			.setAlphaToOneEnable(false);

		vk::PipelineColorBlendAttachmentState color_blend_attachment {};
		color_blend_attachment
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
			.setBlendEnable(false)
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
			.setAlphaBlendOp(vk::BlendOp::eAdd);

		vk::PipelineColorBlendStateCreateInfo color_blending_info {};
		color_blending_info
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachments(color_blend_attachment);
		
#if 0
		std::vector<vk::DynamicState> dynamic_states{ vk::DynamicState::eViewport, vk::DynamicState::eLineWidth };

		vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
		dynamic_state_info
			.setDynamicStates(dynamic_states);
#endif

		vk::PipelineLayoutCreateInfo pipeline_layout_info{};

		m_pPipelineLayout = m_pDevice.createPipelineLayout(pipeline_layout_info);

		vk::GraphicsPipelineCreateInfo graphics_pipeline_info {};
		graphics_pipeline_info
			.setStages(shader.GetPipelineStagesInfo())
			.setPVertexInputState(&vertex_input_info)
			.setPInputAssemblyState(&input_assembly_info)
			.setPViewportState(&viewport_state_info)
			.setPRasterizationState(&rasterization_state_info)
			.setPMultisampleState(&multisampling_info)
			.setPColorBlendState(&color_blending_info);
		graphics_pipeline_info
			.setLayout(m_pPipelineLayout)
			.setRenderPass(window.m_render_pass)
			.setSubpass(0)
			.setBasePipelineHandle(VK_NULL_HANDLE)
			.setBasePipelineIndex(-1);

		auto result = m_pDevice.createGraphicsPipeline(VK_NULL_HANDLE, graphics_pipeline_info);
		if (result.result != vk::Result::eSuccess)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error creating graphics pipeline!");
			exit(1);
		}

		m_pGraphicsPipeline = result.value;

		SDL_Log("Graphics Pipeline created.");
	}

	void Renderer::Init_Framebuffers(RendererWindow& window)
	{
		for (size_t i = 0; i < window.m_image_resources.size(); ++i)
		{
			vk::FramebufferCreateInfo framebuffer_info;
			framebuffer_info
				.setAttachments(window.m_image_resources[i].m_image_view)
				.setRenderPass(window.m_render_pass)
				.setWidth(m_pSwapchainExtent.width)
				.setHeight(m_pSwapchainExtent.height)
				.setLayers(1);

			window.m_image_resources[i].m_framebuffer = m_pDevice.createFramebuffer(framebuffer_info);
			SDL_Log("Created Framebuffer for swapchain image number %d", i);
		}
	}

	void Renderer::Init_CommandPool()
	{
		vk::CommandPoolCreateInfo command_pool_info{};
		command_pool_info
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(m_pQueueFamilyIdx.m_graphicsFamily.value());

		m_pCommandPool = m_pDevice.createCommandPool(command_pool_info);

		SDL_Log("CommandPool created.");

		if (m_pDifferentPresentQueue)
		{
			vk::CommandPoolCreateInfo present_pool_info{};
			present_pool_info
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(m_pQueueFamilyIdx.m_presentFamily.value());

			m_pPresentCommandPool = m_pDevice.createCommandPool(present_pool_info);

			SDL_Log("Separate Present CommandPool created.");
		}
	}

	void Renderer::Init_CommandBuffers(RendererWindow& window)
	{
		vk::CommandBufferAllocateInfo command_buffer_alloc_info {};
		command_buffer_alloc_info
			.setCommandPool(m_pCommandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(2);

		window.m_pCommandBuffers = m_pDevice.allocateCommandBuffers(command_buffer_alloc_info);

		SDL_Log("CommandBuffer Created.");

		if (m_pDifferentPresentQueue)
		{
			vk::CommandBufferAllocateInfo present_buffer_alloc_info{};
			present_buffer_alloc_info
				.setCommandPool(m_pPresentCommandPool)
				.setLevel(vk::CommandBufferLevel::ePrimary)
				.setCommandBufferCount(1);

			window.m_pPresentCommandBuffer = m_pDevice.allocateCommandBuffers(present_buffer_alloc_info)[0];

			SDL_Log("Separate Present CommandBuffer Created.");
		}
	}

	void Renderer::Init_Sychronization(RendererWindow& window)
	{
		for (int i = 0; i < FRAME_LAG; i++)
		{
			window.m_image_available_semaphores[i] = m_pDevice.createSemaphore(vk::SemaphoreCreateInfo{});
		
			window.m_render_finished_semaphores[i] = m_pDevice.createSemaphore(vk::SemaphoreCreateInfo{});

			window.m_in_flight_fences[i] = m_pDevice.createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
		}

		SDL_Log("Created synchronization semaphores and fences");
	}

	void Renderer::Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index)
	{
		RendererWindow& window = m_pWindows[MAIN_WINDOW_ID];
		vk::CommandBufferBeginInfo cmd_buffer_begin_info {};

		cmd_buffer.begin(cmd_buffer_begin_info);

		if (m_pDifferentPresentQueue)
		{
			present_cmd_buffer.begin(cmd_buffer_begin_info);
		}

		vk::ClearValue clear_value (std::array<float, 4>({ {0.2f, 0.2f, 0.2f, 1.f} }));

		vk::RenderPassBeginInfo render_pass_begin_info {};
		render_pass_begin_info
			.setRenderPass(window.m_render_pass)
			.setFramebuffer(window.m_image_resources[image_index].m_framebuffer)
			.setRenderArea(vk::Rect2D{{0, 0}, m_pSwapchainExtent})
			.setClearValues(clear_value);

		cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pGraphicsPipeline);

		cmd_buffer.draw(3, 1, 0, 0);

		cmd_buffer.endRenderPass();

		cmd_buffer.end();
	}

	void Renderer::Draw()
	{
		RendererWindow& window = m_pWindows[MAIN_WINDOW_ID];
		if (m_pDevice.waitForFences(window.m_in_flight_fences[window.current_buffer], true, UINT64_MAX) != vk::Result::eSuccess)
		{
			SDL_Log("Error waiting for In Flight Fence");
			exit(1);
		}

		uint32_t image_index;
		{
			vk::Result result = m_pDevice.acquireNextImageKHR(window.m_swapchain, UINT64_MAX, window.m_image_available_semaphores[window.current_buffer], VK_NULL_HANDLE, &image_index);
			if (result == vk::Result::eErrorOutOfDateKHR)
			{
				Recreate_Swapchain(window);
				return;
			}
			else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
			{
				throw std::runtime_error("Failed to acquire Swapchain image!");
			}
		}

		m_pDevice.resetFences(window.m_in_flight_fences[window.current_buffer]);

		vk::CommandBuffer current_cmd_buffer = window.m_pCommandBuffers[window.current_buffer];

		current_cmd_buffer.reset();

		Record_CommandBuffer(current_cmd_buffer, (m_pDifferentPresentQueue)? window.m_pPresentCommandBuffer : current_cmd_buffer, image_index);

		vk::PipelineStageFlags wait_stage{ vk::PipelineStageFlagBits::eColorAttachmentOutput };

		vk::SubmitInfo submit_info {};
		submit_info
			.setCommandBuffers(window.m_pCommandBuffers[window.current_buffer])
			.setWaitSemaphores(window.m_image_available_semaphores[window.current_buffer])
			.setWaitDstStageMask(wait_stage)
			.setSignalSemaphores(window.m_render_finished_semaphores[window.current_buffer]);

		m_pGraphicsQueue.submit(submit_info, window.m_in_flight_fences[window.current_buffer]);

		vk::PresentInfoKHR present_info{};
		present_info
			.setWaitSemaphores(window.m_render_finished_semaphores[window.current_buffer])
			.setSwapchains(window.m_swapchain)
			.setImageIndices(image_index);

		{
			vk::Result result = m_pPresentationQueue.presentKHR(present_info);
			if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
			{
				Recreate_Swapchain(window);
			}
			else if (result != vk::Result::eSuccess)
			{
				SDL_Log("Presentation Failed.");
				exit(1);
			}
		}

		window.current_buffer = (window.current_buffer + 1) % FRAME_LAG;
	}

	void Renderer::Recreate_Swapchain(RendererWindow& window)
	{
		m_pDevice.waitIdle();

		Init_Swapchain(window);
		Init_Framebuffers(window);
	}

	void Renderer::Cleanup_Swapchain(RendererWindow& window)
	{
		for (int i = 0; i < window.m_image_resources.size(); ++i)
		{
			ImageResources& resource = window.m_image_resources[i];
			if (resource.m_framebuffer)
			{
				m_pDevice.destroyFramebuffer(resource.m_framebuffer);
				resource.m_framebuffer = VK_NULL_HANDLE;
				SDL_Log("Framebuffer of Swapchain Image %d Destroyed.", i);
			}
			if (resource.m_image_view)
			{
				m_pDevice.destroyImageView(resource.m_image_view);
				resource.m_image_view = VK_NULL_HANDLE;
				SDL_Log("ImageView of Swapchain Image %d Destroyed.", i);
			}
		}
		if (window.m_swapchain)
		{
			m_pDevice.destroySwapchainKHR(window.m_swapchain);
			window.m_swapchain = VK_NULL_HANDLE;
			SDL_Log("Swapchain Destroyed.");
		}
	}

	void Renderer::Reset()
	{
		// Destroy Synchronization primitives
		{
			for (RendererWindow& window : m_pWindows)
			{
				for (int i = 0; i < FRAME_LAG; i++)
				{
					if (window.m_image_available_semaphores[i])
					{
						m_pDevice.destroySemaphore(window.m_image_available_semaphores[i]);
						window.m_image_available_semaphores[i] = VK_NULL_HANDLE;
						SDL_Log("ImageAvailable semaphore destroyed.");
					}
					if (window.m_render_finished_semaphores[i])
					{
						m_pDevice.destroySemaphore(window.m_render_finished_semaphores[i]);
						window.m_render_finished_semaphores[i] = VK_NULL_HANDLE;
						SDL_Log("RenderFinish semaphore destroyed.");
					}
					if (window.m_in_flight_fences[i])
					{
						m_pDevice.destroyFence(window.m_in_flight_fences[i]);
						window.m_in_flight_fences[i] = VK_NULL_HANDLE;
						SDL_Log("InFlight fence destroyed.");
					}
				}
			}
		}
		if (m_pCommandPool)
		{
			m_pDevice.destroyCommandPool(m_pCommandPool);
			m_pCommandPool = VK_NULL_HANDLE;
			SDL_Log("CommandPool Destroyed.");
		}
		// Destroy Graphics Pipeline
		if (m_pGraphicsPipeline)
		{
			m_pDevice.destroyPipeline(m_pGraphicsPipeline);
			m_pGraphicsPipeline = VK_NULL_HANDLE;
			SDL_Log("GraphicsPipeline Destroyed.");
		}
		// Destroy Pipeline Layouts
		if (m_pPipelineLayout)
		{
			m_pDevice.destroyPipelineLayout(m_pPipelineLayout);
			m_pPipelineLayout = VK_NULL_HANDLE;
			SDL_Log("PipelineLayout Destroyed.");
		}
		// Destroy Render Pass
		{
			for (RendererWindow& window : m_pWindows)
			{
				if (window.m_render_pass)
				{
					m_pDevice.destroyRenderPass(window.m_render_pass);
					window.m_render_pass = VK_NULL_HANDLE;
					SDL_Log("RenderPass Destroyed.");
				}
			}
		}
		// Destroy Swapchain and its resources (FrameBuffers, Image Views)
		for (RendererWindow& window : m_pWindows)
		{
			Cleanup_Swapchain(window);
		}
		// Destroy Logical Device
		if (m_pDevice)
		{
			m_pDevice.destroy();
			m_pDevice = VK_NULL_HANDLE;
			SDL_Log("Logical Device Destroyed");
		}
		// Destroy Surface
		for (RendererWindow& window : m_pWindows)
		{
			if (window.m_surface)
			{
				m_pVkInstance.destroySurfaceKHR(window.m_surface);
				window.m_surface = VK_NULL_HANDLE;
				SDL_Log("Surface Destroyed");
			}
		}
		// Destroy Vulkan Instance
		if (m_pVkInstance)
		{
			m_pVkInstance.destroy();
			m_pVkInstance = VK_NULL_HANDLE;
			SDL_Log("Instance Destroyed");
		}
	}
}
