#include "Renderer/Renderer.h"

#include "Renderer/VkInitializerHelper.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/Swapchain.h"
#include "Renderer/Shader.h"
#include "Core/Window.h"
#include "Core/PerspectiveCamera.h"
#include "Core/LogSystem.h"
#include "Geometry/Geometry.h"
#include "Scene/Entity.h"
#include "Scene/Components/Mesh3DComponent.h"
#include "Scene/Components/Transform3DComponent.h"


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

	Renderer* Renderer::singleton = nullptr;

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
		if (!render_device_)
		{
			render_device_ = std::make_unique<RenderDevice>(window);
		}

		m_pWindows.resize(m_pWindow_number + 1);
		RendererWindow& rend_window = m_pWindows[m_pWindow_number];
		m_pWindowId_index_map[window->GetWindowID()] = m_pWindow_number;
		m_pWindow_number++;

		rend_window.m_associated_window = window;
		rend_window.swapchain_ = std::make_unique<Swapchain>(render_device_.get(), window);

		// Create UniformBuffers
		Init_UniformBuffers();
		// Create DescriptorPool and the DescriptorSets
		Init_DescriptorSets();

		Init_GraphicsPipeline(rend_window);

		// Initialize CommandBuffers
		Init_CommandBuffers(rend_window);

	}

	void Renderer::Destroy_Window(Window* window)
	{
		if (render_device_)
			render_device_->WaitIdle();
		Reset();
	}

	void Renderer::Init_UniformBuffers()
	{
		vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

		m_pUniform_buffers.reserve(FRAME_LAG);

		BRR_LogInfo("Creating Uniform Buffers");
		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			m_pUniform_buffers.emplace_back(render_device_->Get_VkDevice(), buffer_size,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		}

		BRR_LogInfo("Uniform Buffers created.");
	}

	void Renderer::Init_DescriptorSets()
	{
		m_pDescriptorLayoutCache = new DescriptorLayoutCache(render_device_->Get_VkDevice());
		m_pDescriptorAllocator = new DescriptorAllocator(render_device_->Get_VkDevice());


		DescriptorLayoutBuilder layoutBuilder = DescriptorLayoutBuilder::MakeDescriptorLayoutBuilder(m_pDescriptorLayoutCache);
		layoutBuilder
			.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
	        .SetBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

		DescriptorLayout descriptor_layout = layoutBuilder.BuildDescriptorLayout();
		m_pDescriptorSetLayout = descriptor_layout.m_descriptor_set_layout;
		/*
		std::array<vk::DescriptorBufferInfo, FRAME_LAG> descriptor_buffer_infos;
		for (uint32_t buffer_info_idx = 0; buffer_info_idx < FRAME_LAG; buffer_info_idx++)
		{
			descriptor_buffer_infos[buffer_info_idx] = m_pUniform_buffers[buffer_info_idx].GetDescriptorInfo();
		}

		DescriptorSetBuilder setBuilder = DescriptorSetBuilder<FRAME_LAG>::MakeDescriptorSetBuilder(descriptor_layout, m_pDescriptorAllocator);
		setBuilder.BindBuffer(0, descriptor_buffer_infos);

		std::array<vk::DescriptorSet, FRAME_LAG> descriptor_sets;
		setBuilder.BuildDescriptorSet(descriptor_sets);

		m_pDescriptorSets = {descriptor_sets.begin(), descriptor_sets.end()};

		BRR_LogInfo("Descriptor Sets created.");*/
	}

	void Renderer::Init_GraphicsPipeline(RendererWindow& window)
	{
		Shader shader = Shader::Create_Shader("vert", "frag");

		m_graphics_pipeline.Init_GraphicsPipeline(render_device_->Get_VkDevice(), m_pDescriptorSetLayout, shader, window.swapchain_.get());
	}

	void Renderer::Init_CommandBuffers(RendererWindow& window)
	{
		vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
		command_buffer_alloc_info
			.setCommandPool(render_device_->GetGraphicsCommandPool())
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(2);

		auto allocCmdBufferResult = render_device_->Get_VkDevice().allocateCommandBuffers(command_buffer_alloc_info);
		if (allocCmdBufferResult.result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not allocate CommandBuffer! Result code: {}.", vk::to_string(allocCmdBufferResult.result).c_str());
			exit(1);
		}
		window.m_pCommandBuffers = allocCmdBufferResult.value;

		BRR_LogInfo("CommandBuffer Created.");

		if (render_device_->IsDifferentPresentQueue())
		{
			vk::CommandBufferAllocateInfo present_buffer_alloc_info{};
			present_buffer_alloc_info
				.setCommandPool(render_device_->GetPresentCommandPool())
				.setLevel(vk::CommandBufferLevel::ePrimary)
				.setCommandBufferCount(1);

			auto allocPresentCmdBufferResult = render_device_->Get_VkDevice().allocateCommandBuffers(present_buffer_alloc_info);
			if (allocPresentCmdBufferResult.result != vk::Result::eSuccess)
			{
				BRR_LogError("Could not allocate presentation CommandBuffer! Result code: {}.", vk::to_string(allocPresentCmdBufferResult.result).c_str());
				exit(1);
			}

			window.m_pPresentCommandBuffer = allocPresentCmdBufferResult.value[0];

			BRR_LogInfo("Separate Present CommandBuffer Created.");
		}
	}

	void Renderer::BeginRenderPass_CommandBuffer(RendererWindow& rend_window, vk::CommandBuffer cmd_buffer,
                                                 vk::CommandBuffer present_cmd_buffer, uint32_t image_index) const
    {
		vk::CommandBufferBeginInfo cmd_buffer_begin_info{};

		cmd_buffer.begin(cmd_buffer_begin_info);

		if (render_device_->IsDifferentPresentQueue())
		{
			present_cmd_buffer.begin(cmd_buffer_begin_info);
		}

		vk::ClearValue clear_value(std::array<float, 4>({ {0.2f, 0.2f, 0.2f, 1.f} }));

		vk::RenderPassBeginInfo render_pass_begin_info{};
		render_pass_begin_info
			.setRenderPass(rend_window.swapchain_->GetRender_Pass())
			.setFramebuffer(rend_window.swapchain_->GetFramebuffer(image_index))
			.setRenderArea(vk::Rect2D{ {0, 0}, rend_window.swapchain_->GetSwapchain_Extent() })
			.setClearValues(clear_value);

		cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	}

	void Renderer::BindPipeline_CommandBuffer(RendererWindow& rend_window, const DevicePipeline& pipeline, vk::CommandBuffer cmd_buffer) const
    {
		cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.GetPipeline());

		cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			pipeline.GetPipelineLayout(), 0,
			m_pDescriptorSets[rend_window.swapchain_->GetCurrentBuffer()],
			{});
	}

	void Renderer::EndRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer) const
	{
		cmd_buffer.endRenderPass();

		cmd_buffer.end();
	}

	void Renderer::Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index, Scene* scene)
	{
		RendererWindow& window = m_pWindows[MAIN_WINDOW_ID];
		BeginRenderPass_CommandBuffer(window, cmd_buffer, present_cmd_buffer, image_index);


		scene->m_scene_renderer.Render(cmd_buffer, window.swapchain_->GetCurrentBuffer(), m_graphics_pipeline);
		//BindPipeline_CommandBuffer(window, m_graphics_pipeline, cmd_buffer);

		// Render Scene
		// TODO: Pass this logic to SceneRenderer
		//auto group_3dRender = scene->m_registry_.group<Transform3DComponent, Mesh3DComponent>();

		//uint32_t idx = 0;
		//group_3dRender.each([&](auto entity, Transform3DComponent& transform, Mesh3DComponent& mesh)
		//{
		//	for (Mesh3DComponent::SurfaceData& surface : mesh.surfaces)
		//	{
		//		//BRR_LogInfo("Rendering surface idx %d", idx);
		//		/*surface.Bind(cmd_buffer);
		//		surface.Draw(cmd_buffer);*/
		//	}
		//});

		EndRenderPass_CommandBuffer(cmd_buffer);
	}

	void Renderer::Draw(Window* window)
	{
		RendererWindow& rend_window = m_pWindows[MAIN_WINDOW_ID];

		uint32_t image_index;
		vk::Result result = rend_window.swapchain_->AcquireNextImage(image_index);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			rend_window.swapchain_->Recreate_Swapchain();
			return;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire Swapchain image!");
		}

		Scene* scene = window->GetScene();
		scene->m_scene_renderer.UpdateRenderData(rend_window.swapchain_->GetCurrentBuffer(), m_pUniform_buffers);

		vk::CommandBuffer current_cmd_buffer = rend_window.m_pCommandBuffers[rend_window.swapchain_->GetCurrentBuffer()];

		current_cmd_buffer.reset();

		Record_CommandBuffer(current_cmd_buffer, (render_device_->IsDifferentPresentQueue())? rend_window.m_pPresentCommandBuffer : current_cmd_buffer, image_index, window->GetScene());

		Update_UniformBuffers(rend_window, *scene);

		rend_window.swapchain_->SubmitCommandBuffer(current_cmd_buffer, image_index);
	}

	void Renderer::Update_UniformBuffers(RendererWindow& window, Scene& scene)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		float aspect = window.swapchain_->GetSwapchain_Extent().width / (float)window.swapchain_->GetSwapchain_Extent().height;

		UniformBufferObject ubo;
		ubo.projection_view = scene.GetMainCamera()->GetProjectionMatrix() * scene.GetMainCamera()->GetViewMatrix();

		m_pUniform_buffers[window.swapchain_->GetCurrentBuffer()].Map();
		m_pUniform_buffers[window.swapchain_->GetCurrentBuffer()].WriteToBuffer(&ubo, sizeof(ubo));
		m_pUniform_buffers[window.swapchain_->GetCurrentBuffer()].Unmap();
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

			 auto createBufferResult = render_device_->Get_VkDevice().createBuffer(buffer_create_info);
			 if (createBufferResult.result != vk::Result::eSuccess)
			 {
				 BRR_LogError("Could not create Buffer! Result code: {}.", vk::to_string(createBufferResult.result).c_str());
				 exit(1);
			 }
			 buffer = createBufferResult.value;

			BRR_LogInfo("Buffer created.");
		}

		// Allocate Memory
		{
			vk::MemoryRequirements memory_requirements = render_device_->Get_VkDevice().getBufferMemoryRequirements(buffer);

			vk::MemoryAllocateInfo allocate_info{};
			allocate_info
				.setAllocationSize(memory_requirements.size)
				.setMemoryTypeIndex(FindMemoryType(memory_requirements.memoryTypeBits,
					properties));

			 auto allocMemResult = render_device_->Get_VkDevice().allocateMemory(allocate_info);
			 if (allocMemResult.result != vk::Result::eSuccess)
			 {
				 BRR_LogError("Could not allocate DeviceMemory for buffer! Result code: {}.", vk::to_string(allocMemResult.result).c_str());
				 exit(1);
			 }
			 buffer_memory = allocMemResult.value;

			BRR_LogInfo("Buffer Memory Allocated.");
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

		 auto allocCmdBuffersResult = render_device_->Get_VkDevice().allocateCommandBuffers(cmd_buffer_alloc_info);
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

	void Renderer::Reset()
	{
		// Destroy Windows Swapchain and its Resources
		{
			for (RendererWindow& window : m_pWindows)
			{
				window.swapchain_ = nullptr;
			}
		}

		// Destroy Uniform Buffers
		if (!m_pUniform_buffers.empty())
		{
			m_pUniform_buffers.clear();
		}

		m_graphics_pipeline.DestroyPipeline();

		render_device_ = nullptr;

		BRR_LogInfo("Renderer Destroyed");
	}

    Renderer::Renderer()
    {
		if (singleton)
		{
			BRR_LogError("Another Renderer was created. Renderer class should be unique.");
			exit(1);
		}
		singleton = this;
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

    DescriptorLayoutBuilder Renderer::GetDescriptorLayoutBuilder() const
    {
		return DescriptorLayoutBuilder::MakeDescriptorLayoutBuilder(m_pDescriptorLayoutCache);
    }

    DescriptorSetBuilder<FRAME_LAG> Renderer::GetDescriptorSetBuilder(const DescriptorLayout& layout) const
    {
		return DescriptorSetBuilder<FRAME_LAG>::MakeDescriptorSetBuilder(layout, m_pDescriptorAllocator);
    }
}
