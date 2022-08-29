#include "Core/Window.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/Swapchain.h"
#include "Renderer/Geometry/Geometry.h"
#include "Renderer/Shader.h"

namespace brr::render
{
	static const std::vector<Vertex2_PosColor> vertices{
		{glm::vec2{-0.5f, -0.5f}, glm::vec3{1.f, 0.f, 0.f}},
		{glm::vec2{0.5f, -0.5f}, glm::vec3{0.f, 1.f, 0.f}},
		{glm::vec2{.5f, .5f}, glm::vec3{0.f, 0.f, 1.f}},
		{glm::vec2{-.5f, .5f}, glm::vec3{1.f, 1.f, 1.f}}
	};

	static const std::vector<uint32_t> indices
	{
		0, 1, 2, 2, 3, 0
	};

	struct ModelMatrixPushConstant
	{
		glm::mat4 model_matrix;
	};

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

		if (!render_device_)
		{
			render_device_ = std::make_unique<RenderDevice>(window);
		}

		rend_window.swapchain_ = std::make_unique<Swapchain>(render_device_.get(), window);

		// Define the DescriptorSetLayout and Initialize the GraphicsPipeline
		Init_DescriptorSetLayout();
		Init_GraphicsPipeline(rend_window);

		// Initialize CommandBuffers
		Init_CommandBuffers(rend_window);

		// Create UniformBuffers, DescriptorPool and the DescriptorSets
		Init_UniformBuffers();
		Init_DescriptorPool();
		Init_DescriptorSets();

		std::vector<Vertex2_PosColor> verts = vertices;
		std::vector<uint32_t> inds = indices;
		mesh = new Mesh2D(render_device_->Get_VkDevice(), std::move(verts), std::move(inds));
	}

	void Renderer::Destroy_Window(Window* window)
	{
		if (render_device_)
			render_device_->WaitIdle();
		Reset();
	}

	void Renderer::Init_DescriptorSetLayout()
	{
		vk::DescriptorSetLayoutBinding ubo_LayoutBinding{};
		ubo_LayoutBinding
			.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex)
			.setPImmutableSamplers(nullptr);

		vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
		descriptor_set_layout_create_info
			.setBindings(ubo_LayoutBinding);

		m_pDescriptorSetLayout = render_device_->Get_VkDevice().createDescriptorSetLayout(descriptor_set_layout_create_info);
	}

	void Renderer::Init_GraphicsPipeline(RendererWindow& window)
	{
		Shader shader = Shader::Create_Shader("vert", "frag");

		vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
		{
			vk::VertexInputBindingDescription binding_description = Vertex2_PosColor::GetBindingDescription();
			std::array<vk::VertexInputAttributeDescription, 2> attribute_descriptions = Vertex2_PosColor::GetAttributeDescriptions();
			
			vertex_input_info
				.setVertexBindingDescriptions(binding_description)
				.setVertexAttributeDescriptions(attribute_descriptions);
		}

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false);

		vk::Extent2D swapchain_extent = window.swapchain_->GetSwapchain_Extent();

		vk::Viewport viewport {};
		viewport
			.setX(0.f)
			.setY(0.f)
			.setWidth((float)swapchain_extent.width)
			.setHeight((float)swapchain_extent.height)
			.setMinDepth(0.f)
			.setMaxDepth(1.f);

		vk::Rect2D scissor{ {0, 0}, swapchain_extent };

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
		pipeline_layout_info
			.setSetLayouts(m_pDescriptorSetLayout);
			//.setPushConstantRanges(vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof()});

		m_pPipelineLayout = render_device_->Get_VkDevice().createPipelineLayout(pipeline_layout_info);

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
			.setRenderPass(window.swapchain_->GetRender_Pass())
			.setSubpass(0)
			.setBasePipelineHandle(VK_NULL_HANDLE)
			.setBasePipelineIndex(-1);

		auto result = render_device_->Get_VkDevice().createGraphicsPipeline(VK_NULL_HANDLE, graphics_pipeline_info);
		if (result.result != vk::Result::eSuccess)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error creating graphics pipeline!");
			exit(1);
		}

		m_pGraphicsPipeline = result.value;

		SDL_Log("Graphics Pipeline created.");
	}

	void Renderer::Init_UniformBuffers()
	{
		vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

		uniform_buffers_.reserve(FRAME_LAG);

		SDL_Log("Creating Uniform Buffers");
		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			uniform_buffers_.emplace_back(render_device_->Get_VkDevice(), buffer_size,
			                              vk::BufferUsageFlagBits::eUniformBuffer,
			                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		}

		SDL_Log("Uniform Buffers created.");
	}

	void Renderer::Init_DescriptorPool()
	{
		vk::DescriptorPoolSize pool_size{};
		pool_size
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(static_cast<uint32_t>(FRAME_LAG));

		vk::DescriptorPoolCreateInfo pool_create_info{};
		pool_create_info
			.setPoolSizes(pool_size)
			.setMaxSets(FRAME_LAG);

		m_pDescriptorPool = render_device_->Get_VkDevice().createDescriptorPool(pool_create_info);

		SDL_Log("Descriptor Pool created.");
	}

	void Renderer::Init_DescriptorSets()
	{
		std::vector<vk::DescriptorSetLayout> layouts(FRAME_LAG, m_pDescriptorSetLayout);

		vk::DescriptorSetAllocateInfo desc_set_allocate_info{};
		desc_set_allocate_info
			.setDescriptorPool(m_pDescriptorPool)
			.setSetLayouts(layouts);

		m_pDescriptorSets = render_device_->Get_VkDevice().allocateDescriptorSets(desc_set_allocate_info);

		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			vk::DescriptorBufferInfo desc_buffer_info = uniform_buffers_[i].GetDescriptorInfo();

			vk::WriteDescriptorSet descriptor_write{};
			descriptor_write
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				.setDstSet(m_pDescriptorSets[i])
				.setDstBinding(0)
				.setDstArrayElement(0)
				.setBufferInfo(desc_buffer_info);

			render_device_->Get_VkDevice().updateDescriptorSets(descriptor_write, {});
		}

		SDL_Log("Descriptor Sets created.");
	}

	void Renderer::Init_CommandPool()
	{
		vk::CommandPoolCreateInfo command_pool_info{};
		command_pool_info
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(render_device_->GetQueueFamilyIndices().m_graphicsFamily.value());

		m_pCommandPool = render_device_->Get_VkDevice().createCommandPool(command_pool_info);

		SDL_Log("CommandPool created.");

		if (render_device_->IsDifferentPresentQueue())
		{
			vk::CommandPoolCreateInfo present_pool_info{};
			present_pool_info
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(render_device_->GetQueueFamilyIndices().m_presentFamily.value());

			m_pPresentCommandPool = render_device_->Get_VkDevice().createCommandPool(present_pool_info);

			SDL_Log("Separate Present CommandPool created.");
		}

		if (render_device_->IsDifferentTransferQueue())
		{
			vk::CommandPoolCreateInfo transfer_pool_info{};
			transfer_pool_info
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
				.setQueueFamilyIndex(render_device_->GetQueueFamilyIndices().m_transferFamily.value());

			m_pTransferCommandPool = render_device_->Get_VkDevice().createCommandPool(transfer_pool_info);

			SDL_Log("Separate Transfer CommandPool created.");
		}
	}

	void Renderer::Init_CommandBuffers(RendererWindow& window)
	{
		vk::CommandBufferAllocateInfo command_buffer_alloc_info {};
		command_buffer_alloc_info
			.setCommandPool(render_device_->GetGraphicsCommandPool())
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(2);

		window.m_pCommandBuffers = render_device_->Get_VkDevice().allocateCommandBuffers(command_buffer_alloc_info);

		SDL_Log("CommandBuffer Created.");

		if (render_device_->IsDifferentPresentQueue())
		{
			vk::CommandBufferAllocateInfo present_buffer_alloc_info{};
			present_buffer_alloc_info
				.setCommandPool(render_device_->GetPresentCommandPool())
				.setLevel(vk::CommandBufferLevel::ePrimary)
				.setCommandBufferCount(1);

			window.m_pPresentCommandBuffer = render_device_->Get_VkDevice().allocateCommandBuffers(present_buffer_alloc_info)[0];

			SDL_Log("Separate Present CommandBuffer Created.");
		}
	}

	void Renderer::Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index)
	{
		RendererWindow& window = m_pWindows[MAIN_WINDOW_ID];
		vk::CommandBufferBeginInfo cmd_buffer_begin_info {};

		cmd_buffer.begin(cmd_buffer_begin_info);

		if (render_device_->IsDifferentPresentQueue())
		{
			present_cmd_buffer.begin(cmd_buffer_begin_info);
		}

		vk::ClearValue clear_value (std::array<float, 4>({ {0.2f, 0.2f, 0.2f, 1.f} }));

		vk::RenderPassBeginInfo render_pass_begin_info {};
		render_pass_begin_info
			.setRenderPass(window.swapchain_->GetRender_Pass())
			.setFramebuffer(window.swapchain_->GetFramebuffer(image_index))
			.setRenderArea(vk::Rect2D{{0, 0}, window.swapchain_->GetSwapchain_Extent()})
			.setClearValues(clear_value);

		cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pGraphicsPipeline);

		cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, 
			m_pPipelineLayout, 0, 
			m_pDescriptorSets[window.swapchain_->GetCurrentBuffer()], 
			{});

		mesh->Bind(cmd_buffer);
		mesh->Draw(cmd_buffer);

		cmd_buffer.endRenderPass();

		cmd_buffer.end();
	}

	void Renderer::Draw()
	{
		RendererWindow& window = m_pWindows[MAIN_WINDOW_ID];

		uint32_t image_index;
		vk::Result result = window.swapchain_->AcquireNextImage(image_index);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			window.swapchain_->Recreate_Swapchain();
			return;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire Swapchain image!");
		}

		vk::CommandBuffer current_cmd_buffer = window.m_pCommandBuffers[window.swapchain_->GetCurrentBuffer()];

		current_cmd_buffer.reset();

		Record_CommandBuffer(current_cmd_buffer, (render_device_->IsDifferentPresentQueue())? window.m_pPresentCommandBuffer : current_cmd_buffer, image_index);

		Update_UniformBuffers(window);

		window.swapchain_->SubmitCommandBuffer(current_cmd_buffer, image_index);
	}

	void Renderer::Create_Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer, vk::DeviceMemory& buffer_memory)
	{
		// Create Buffer
		{
			vk::SharingMode sharing_mode = render_device_->IsDifferentTransferQueue() ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
			std::vector<uint32_t> indices{
				render_device_->GetQueueFamilyIndices().m_graphicsFamily.value(),
				render_device_->GetQueueFamilyIndices().m_transferFamily.value()
			};

			vk::BufferCreateInfo buffer_create_info;
			buffer_create_info
				.setUsage(usage)
				.setSharingMode(sharing_mode)
				.setSize(buffer_size);
			if (sharing_mode == vk::SharingMode::eConcurrent)
			{
			
				buffer_create_info.setQueueFamilyIndices(indices);
			}

			buffer = render_device_->Get_VkDevice().createBuffer(buffer_create_info);

			SDL_Log("Buffer created.");
		}

		// Allocate Memory
		{
			vk::MemoryRequirements memory_requirements = render_device_->Get_VkDevice().getBufferMemoryRequirements(buffer);

			vk::MemoryAllocateInfo allocate_info{};
			allocate_info
				.setAllocationSize(memory_requirements.size)
				.setMemoryTypeIndex(FindMemoryType(memory_requirements.memoryTypeBits,
					properties));

			buffer_memory = render_device_->Get_VkDevice().allocateMemory(allocate_info);

			SDL_Log("Buffer Memory Allocated.");
		}

		render_device_->Get_VkDevice().bindBufferMemory(buffer, buffer_memory, 0);

		return;
	}

	void Renderer::Copy_Buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size, 
		vk::DeviceSize src_buffer_offset, vk::DeviceSize dst_buffer_offset)
	{
		const vk::CommandPool transfer_cmd_pool = (render_device_->IsDifferentTransferQueue()) ? render_device_->GetTransferCommandPool() : render_device_->GetGraphicsCommandPool();

		vk::CommandBufferAllocateInfo cmd_buffer_alloc_info{};
		cmd_buffer_alloc_info
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandPool(transfer_cmd_pool)
			.setCommandBufferCount(1);

		vk::CommandBuffer cmd_buffer = render_device_->Get_VkDevice().allocateCommandBuffers(cmd_buffer_alloc_info)[0];

		vk::CommandBufferBeginInfo cmd_begin_info{};
		cmd_begin_info
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		cmd_buffer.begin(cmd_begin_info);

		vk::BufferCopy copy_region{};
		copy_region
			.setSrcOffset(0)
			.setDstOffset(0)
			.setSize(size);

		cmd_buffer.copyBuffer(src_buffer, dst_buffer, copy_region);

		cmd_buffer.end();

		vk::SubmitInfo submit_info;
		submit_info
			.setCommandBufferCount(1)
			.setCommandBuffers(cmd_buffer);

		render_device_->GetTransferQueue().submit(submit_info);
		render_device_->GetTransferQueue().waitIdle();

		render_device_->Get_VkDevice().freeCommandBuffers(transfer_cmd_pool, cmd_buffer);
	}

	void Renderer::Recreate_Swapchain(RendererWindow& window)
	{
		render_device_->WaitIdle();

		window.swapchain_->Recreate_Swapchain();
	}

	void Renderer::Update_UniformBuffers(RendererWindow& window)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		float aspect = window.swapchain_->GetSwapchain_Extent().width / (float)window.swapchain_->GetSwapchain_Extent().height;

		UniformBufferObject ubo {};
		glm::mat4 perspective = glm::perspective(glm::radians(45.f), aspect, 0.1f, 10.f);
		perspective[1][1] *= -1;
		ubo.projection_view = perspective * glm::lookAt(glm::vec3{ 2.f }, glm::vec3{ 0.f }, glm::vec3{ 0.f, 0.f, 1.f });

		uniform_buffers_[window.swapchain_->GetCurrentBuffer()].Map();
		uniform_buffers_[window.swapchain_->GetCurrentBuffer()].WriteToBuffer(&ubo, sizeof(ubo));
		uniform_buffers_[window.swapchain_->GetCurrentBuffer()].Unmap();
	}

	void Renderer::Reset()
	{
		delete mesh;
		mesh = nullptr;
		// Destroy Synchronization primitives
		{
			for (RendererWindow& window : m_pWindows)
			{
				window.swapchain_ = nullptr;
				/*for (int i = 0; i < FRAME_LAG; i++)
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
				}*/
			}
		}
		//if (m_pCommandPool)
		//{
		//	m_pDevice.destroyCommandPool(m_pCommandPool);
		//	m_pCommandPool = VK_NULL_HANDLE;
		//	SDL_Log("CommandPool Destroyed.");
		//}
		//if (m_pDifferentPresentQueue && m_pPresentCommandPool)
		//{
		//	m_pDevice.destroyCommandPool(m_pPresentCommandPool);
		//	m_pPresentCommandPool = VK_NULL_HANDLE;
		//	SDL_Log("Separate Present CommandPool Destroyed.");
		//}
		//if (m_pDifferentTransferQueue && m_pTransferCommandPool)
		//{
		//	m_pDevice.destroyCommandPool(m_pTransferCommandPool);
		//	m_pTransferCommandPool = VK_NULL_HANDLE;
		//	SDL_Log("Separate Transfer CommandPool Destroyed.");
		//}
		//// Destroy Graphics Pipeline
		//if (m_pGraphicsPipeline)
		//{
		//	m_pDevice.destroyPipeline(m_pGraphicsPipeline);
		//	m_pGraphicsPipeline = VK_NULL_HANDLE;
		//	SDL_Log("GraphicsPipeline Destroyed.");
		//}
		//// Destroy Pipeline Layouts
		//if (m_pPipelineLayout)
		//{
		//	m_pDevice.destroyPipelineLayout(m_pPipelineLayout);
		//	m_pPipelineLayout = VK_NULL_HANDLE;
		//	SDL_Log("PipelineLayout Destroyed.");
		//}
		// Destroy Uniform Buffers
		if (!uniform_buffers_.empty())
		{
			uniform_buffers_.clear();
		}

		render_device_ = nullptr;
		// Destroy Descriptor Pool
		//if (m_pDescriptorPool)
		//{
		//	m_pDevice.destroyDescriptorPool(m_pDescriptorPool);
		//	m_pDescriptorPool = VK_NULL_HANDLE;
		//	SDL_Log("Descriptor Pool Destroyed.");
		//}
		//// Destroy Descriptor Set Layout
		//if (m_pDescriptorSetLayout)
		//{
		//	m_pDevice.destroyDescriptorSetLayout(m_pDescriptorSetLayout);
		//	m_pDescriptorSetLayout = VK_NULL_HANDLE;
		//	SDL_Log("Descriptor Set Layout Destroyed.");
		//}
		//// Destroy Render Pass
		//{
		//	for (RendererWindow& window : m_pWindows)
		//	{
		//		if (window.m_render_pass)
		//		{
		//			m_pDevice.destroyRenderPass(window.m_render_pass);
		//			window.m_render_pass = VK_NULL_HANDLE;
		//			SDL_Log("RenderPass Destroyed.");
		//		}
		//	}
		//}
		//// Destroy Swapchain and its resources (FrameBuffers, Image Views)
		//for (RendererWindow& window : m_pWindows)
		//{
		//	Cleanup_Swapchain(window);
		//}
		//// Destroy Logical Device
		//if (m_pDevice)
		//{
		//	m_pDevice.destroy();
		//	m_pDevice = VK_NULL_HANDLE;
		//	SDL_Log("Logical Device Destroyed");
		//}
		//// Destroy Surface
		//for (RendererWindow& window : m_pWindows)
		//{
		//	if (window.m_surface)
		//	{
		//		m_pVkInstance.destroySurfaceKHR(window.m_surface);
		//		window.m_surface = VK_NULL_HANDLE;
		//		SDL_Log("Surface Destroyed");
		//	}
		//}
		//// Destroy Vulkan Instance
		//if (m_pVkInstance)
		//{
		//	m_pVkInstance.destroy();
		//	m_pVkInstance = VK_NULL_HANDLE;
		//	SDL_Log("Instance Destroyed");
		//}
	}

	uint32_t Renderer::FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties) const
	{
		vk::PhysicalDeviceMemoryProperties pDev_memory_properties = render_device_->Get_PhysicalDevice().getMemoryProperties();
		for (uint32_t i = 0; i < pDev_memory_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) &&
				(pDev_memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find valid memory type.");
	}
}
