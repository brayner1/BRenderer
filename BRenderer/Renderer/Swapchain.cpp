#include "Swapchain.h"

#include <Renderer/Vulkan/VkInitializerHelper.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Visualization/Window.h>
#include <Core/LogSystem.h>


namespace brr::render
{
	Swapchain::Swapchain(vis::Window* window) : m_render_device(VKRD::GetSingleton()), m_window(window)
	{
		Init_Swapchain(window);
		Init_SwapchainResources();
		Init_RenderPass();
		Init_Framebuffers();
		Init_Synchronization();
	}

	vk::Result Swapchain::AcquireNextImage(vk::Semaphore& image_available_semaphore)
	{
		if (m_render_device->Get_VkDevice().waitForFences(m_in_flight_fences[m_current_buffer_idx], true, UINT64_MAX) != vk::Result::eSuccess)
		{
			BRR_LogError("Error waiting for In Flight Fence");
			exit(1);
		}

		m_render_device->Get_VkDevice().resetFences(m_in_flight_fences[m_current_buffer_idx]);

		vk::Result result = m_render_device->Get_VkDevice().acquireNextImageKHR(m_swapchain, UINT64_MAX,
				m_image_available_semaphores[m_current_buffer_idx], VK_NULL_HANDLE, &m_current_image_idx);

		image_available_semaphore = m_image_available_semaphores[m_current_buffer_idx];

		return result;
	}

	vk::Result Swapchain::PresentCurrentImage(vk::Semaphore wait_semaphore)
	{
		vk::PresentInfoKHR present_info{};
		present_info
			.setWaitSemaphores(wait_semaphore)
			.setSwapchains(m_swapchain)
			.setImageIndices(m_current_image_idx);

		vk::Result result = m_render_device->GetPresentQueue().presentKHR(present_info);
		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
		{
			Recreate_Swapchain();
		}
		else if (result != vk::Result::eSuccess)
		{
			BRR_LogError("Presentation Failed.");
			exit(1);
		}

		m_current_buffer_idx = (m_current_buffer_idx + 1) % FRAME_LAG;

		return result;
	}

	Swapchain::~Swapchain()
	{
		Cleanup_Swapchain();

		m_render_device->Get_VkDevice().destroyRenderPass(m_render_pass);
		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			m_render_device->Get_VkDevice().destroySemaphore(m_image_available_semaphores[i]);
			m_render_device->Get_VkDevice().destroyFence(m_in_flight_fences[i]);
		}
	}

	void Swapchain::Init_Swapchain(vis::Window* window)
	{
		vk::SurfaceKHR surface = window->GetVulkanSurface(m_render_device->Get_VkInstance());
		VkHelpers::SwapChainProperties properties = VkHelpers::Query_SwapchainProperties(m_render_device->Get_VkPhysicalDevice(), surface);

		vk::SurfaceFormatKHR surface_format = VkHelpers::Select_SwapchainFormat(properties.m_surfFormats);
		vk::PresentModeKHR present_mode = VkHelpers::Select_SwapchainPresentMode(properties.m_presentModes);
		m_swapchain_extent = VkHelpers::Select_SwapchainExtent(window, properties.m_surfCapabilities);
		m_swapchain_image_format = surface_format.format;

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

		vk::SwapchainCreateInfoKHR swapchain_create_info{};
		swapchain_create_info
			.setSurface(surface)
			.setMinImageCount(imageCount)
			.setImageFormat(m_swapchain_image_format)
			.setImageColorSpace(surface_format.colorSpace)
			.setImageExtent(m_swapchain_extent)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setPresentMode(present_mode)
			.setClipped(true)
			.setPreTransform(preTransform)
			.setCompositeAlpha(compositeAlpha)
			.setOldSwapchain(m_swapchain);

		if (!m_render_device->IsDifferentPresentQueue())
		{
			swapchain_create_info.setImageSharingMode(vk::SharingMode::eExclusive);
		}
		else
		{
			uint32_t graphics_family_queue = m_render_device->GetQueueFamilyIndices().m_graphicsFamily.value();
			uint32_t presentation_family_queue = m_render_device->GetQueueFamilyIndices().m_presentFamily.value();
			std::vector<uint32_t> queue_family_indices{ graphics_family_queue, presentation_family_queue };
			swapchain_create_info
				.setImageSharingMode(vk::SharingMode::eConcurrent)
				.setQueueFamilyIndices(queue_family_indices);
		}

		 auto createSwapchainResult = m_render_device->Get_VkDevice().createSwapchainKHR(swapchain_create_info);
		 if (createSwapchainResult.result != vk::Result::eSuccess)
		 {
			 BRR_LogError("Could not create SwapchainKHR! Result code: {}.", vk::to_string(createSwapchainResult.result).c_str());
			 exit(1);
		 }
		 vk::SwapchainKHR new_swapchain = createSwapchainResult.value;
		BRR_LogInfo("Swapchain created");

		// If old swapchain is valid, destroy it. (It happens on swapchain recreation)
		if (m_swapchain)
		{
			BRR_LogInfo("Swapchain was recreated. Cleaning old swapchain.");
			Cleanup_Swapchain();
		}

		// Assign the new swapchain to the window
		m_swapchain = new_swapchain;
	}

	void Swapchain::Init_SwapchainResources()
	{
		// Acquire swapchain images and create ImageViews
		{
			auto swapchainImagesKHRResult = m_render_device->Get_VkDevice().getSwapchainImagesKHR(m_swapchain);
			std::vector<vk::Image> swapchain_images = swapchainImagesKHRResult.value;
			m_image_resources.resize(swapchain_images.size());
			for (uint32_t i = 0; i < m_image_resources.size(); i++)
			{
				m_image_resources[i].m_image = swapchain_images[i];

				vk::ImageViewCreateInfo image_view_create_info{};
				image_view_create_info
					.setImage(swapchain_images[i])
					.setViewType(vk::ImageViewType::e2D)
					.setFormat(m_swapchain_image_format)
					.setSubresourceRange(vk::ImageSubresourceRange{}
						.setAspectMask(vk::ImageAspectFlagBits::eColor)
						.setBaseMipLevel(0)
						.setLevelCount(1)
						.setLayerCount(1)
						.setBaseArrayLayer(0)
					);

				 auto createImgViewResult = m_render_device->Get_VkDevice().createImageView(image_view_create_info);
				 if (createImgViewResult.result != vk::Result::eSuccess)
				 {
					 BRR_LogError("Could not create ImageView for swapchain! Result code: {}.", vk::to_string(createImgViewResult.result).c_str());
					 exit(1);
				 }
				 m_image_resources[i].m_image_view = createImgViewResult.value;
			}
		}

		BRR_LogInfo("Images Resources initialized.");
	}

	void Swapchain::Init_RenderPass()
	{
		vk::AttachmentDescription color_attachment{};
		color_attachment
			.setFormat(m_swapchain_image_format)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

		vk::AttachmentReference color_attachment_ref{};
		color_attachment_ref
			.setAttachment(0)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription subpass_description{};
		subpass_description
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachments(color_attachment_ref);

		vk::SubpassDependency subpass_dependency[1];
		subpass_dependency[0]
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlagBits::eNone)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite/* | vk::AccessFlagBits::eColorAttachmentRead*/)
			.setDependencyFlags(vk::DependencyFlags());
		//subpass_dependency[1]
		//	.setSrcSubpass(0)
		//	.setDstSubpass(0)
		//	.setSrcStageMask(vk::PipelineStageFlagBits::eTopOfPipe)
		//	.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite/* | vk::AccessFlagBits::eColorAttachmentRead*/)
		//	.setDstStageMask(vk::PipelineStageFlagBits::eVertexInput)
		//	.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
		//	.setDependencyFlags(vk::DependencyFlags());

		vk::RenderPassCreateInfo render_pass_info{};
		render_pass_info
			.setAttachments(color_attachment)
			.setSubpasses(subpass_description)
			.setDependencies(subpass_dependency);

		 auto createRenderPassResult = m_render_device->Get_VkDevice().createRenderPass(render_pass_info);
		 if (createRenderPassResult.result != vk::Result::eSuccess)
		 {
			 BRR_LogError("Could not create RenderPass for swapchain! Result code: {}.", vk::to_string(createRenderPassResult.result).c_str());
			 exit(1);
		 }
		 m_render_pass = createRenderPassResult.value;

		BRR_LogInfo("Render Pass Created");
	}

	void Swapchain::Init_Framebuffers()
	{
		for (size_t i = 0; i < m_image_resources.size(); ++i)
		{
			vk::FramebufferCreateInfo framebuffer_info;
			framebuffer_info
				.setAttachments(m_image_resources[i].m_image_view)
				.setRenderPass(m_render_pass)
				.setWidth(m_swapchain_extent.width)
				.setHeight(m_swapchain_extent.height)
				.setLayers(1);

			 auto createFramebufferResult = m_render_device->Get_VkDevice().createFramebuffer(framebuffer_info);
			 if (createFramebufferResult.result != vk::Result::eSuccess)
			 {
				 BRR_LogError("Could not create Framebuffer for swapchain! Result code: {}.", vk::to_string(createFramebufferResult.result).c_str());
				 exit(1);
			 }
			 m_image_resources[i].m_framebuffer = createFramebufferResult.value;
			BRR_LogInfo("Created Framebuffer for swapchain image number {}", i);
		}
	}

	void Swapchain::Init_Synchronization()
	{
		for (int i = 0; i < FRAME_LAG; i++)
		{
			 auto createImgAvailableSemaphoreResult = m_render_device->Get_VkDevice().createSemaphore(vk::SemaphoreCreateInfo{});
			 if (createImgAvailableSemaphoreResult.result != vk::Result::eSuccess)
			 {
				 BRR_LogError("Could not create Image Available Semaphore for swapchain! Result code: {}.", vk::to_string(createImgAvailableSemaphoreResult.result).c_str());
				 exit(1);
			 }
			 m_image_available_semaphores[i] = createImgAvailableSemaphoreResult.value;

			 auto createInFlightFenceResult = m_render_device->Get_VkDevice().createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
			 if (createInFlightFenceResult.result != vk::Result::eSuccess)
			 {
				 BRR_LogError("Could not create In Flight Fence for swapchain! Result code: {}.", vk::to_string(createInFlightFenceResult.result).c_str());
				 exit(1);
			 }
			 m_in_flight_fences[i] = createInFlightFenceResult.value;
		}

		BRR_LogInfo("Created Swapchain synchronization semaphores and fences");
	}

	void Swapchain::Recreate_Swapchain()
	{
		m_render_device->WaitIdle();

		Init_Swapchain(m_window);
		Init_SwapchainResources();
		Init_Framebuffers();
	}

	void Swapchain::Cleanup_Swapchain()
	{
		for (int i = 0; i < m_image_resources.size(); ++i)
		{
			ImageResources& resource = m_image_resources[i];
			if (resource.m_framebuffer)
			{
				m_render_device->Get_VkDevice().destroyFramebuffer(resource.m_framebuffer);
				resource.m_framebuffer = VK_NULL_HANDLE;
				BRR_LogInfo("Framebuffer of Swapchain Image {} Destroyed.", i);
			}
			if (resource.m_image_view)
			{
				m_render_device->Get_VkDevice().destroyImageView(resource.m_image_view);
				resource.m_image_view = VK_NULL_HANDLE;
				BRR_LogInfo("ImageView of Swapchain Image {} Destroyed.", i);
			}
		}
		if (m_swapchain)
		{
			m_render_device->Get_VkDevice().destroySwapchainKHR(m_swapchain);
			m_swapchain = VK_NULL_HANDLE;
			BRR_LogInfo("Swapchain Destroyed.");
		}
	}
}
