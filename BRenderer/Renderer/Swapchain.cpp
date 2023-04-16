#include "Renderer/Swapchain.h"

#include "RenderDevice.h"
#include "Core/Window.h"
#include "Renderer/VkInitializerHelper.h"

namespace brr::render
{
	Swapchain::Swapchain(RenderDevice* device, Window* window) : device_(device), window_(window)
	{
		Init_Swapchain(window);
		Init_SwapchainResources();
		Init_RenderPass();
		Init_Framebuffers();
		Init_Synchronization();
	}

	vk::Result Swapchain::AcquireNextImage(uint32_t& out_image_index)
	{
		if (device_->Get_VkDevice().waitForFences(in_flight_fences_[current_buffer_], true, UINT64_MAX) != vk::Result::eSuccess)
		{
			SDL_Log("Error waiting for In Flight Fence");
			exit(1);
		}

		vk::Result result = device_->Get_VkDevice().acquireNextImageKHR(swapchain_, UINT64_MAX,
				image_available_semaphores_[current_buffer_], VK_NULL_HANDLE, &out_image_index);

		// Leave the error treatment to the caller
		/*if (result == vk::Result::eErrorOutOfDateKHR)
		{
			Recreate_Swapchain();
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire Swapchain image!");
		}*/

		return result;
	}

	vk::Result Swapchain::SubmitCommandBuffer(vk::CommandBuffer command_buffer, uint32_t image_index)
	{
		vk::PipelineStageFlags wait_stage{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submit_info{};
		submit_info
			.setCommandBuffers(command_buffer)
			.setWaitSemaphores(image_available_semaphores_[current_buffer_])
			.setWaitDstStageMask(wait_stage)
			.setSignalSemaphores(render_finished_semaphores_[current_buffer_]);

		device_->Get_VkDevice().resetFences(in_flight_fences_[current_buffer_]);

		device_->GetGraphicsQueue().submit(submit_info, in_flight_fences_[current_buffer_]);

		vk::PresentInfoKHR present_info{};
		present_info
			.setWaitSemaphores(render_finished_semaphores_[current_buffer_])
			.setSwapchains(swapchain_)
			.setImageIndices(image_index);

		vk::Result result = device_->GetPresentQueue().presentKHR(present_info);
		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
		{
			Recreate_Swapchain();
		}
		else if (result != vk::Result::eSuccess)
		{
			SDL_Log("Presentation Failed.");
			exit(1);
		}

		current_buffer_ = (current_buffer_ + 1) % FRAME_LAG;

		return result;
	}

	Swapchain::~Swapchain()
	{
		Cleanup_Swapchain();

		device_->Get_VkDevice().destroyRenderPass(render_pass_);
		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			device_->Get_VkDevice().destroySemaphore(image_available_semaphores_[i]);
			device_->Get_VkDevice().destroySemaphore(render_finished_semaphores_[i]);
			device_->Get_VkDevice().destroyFence(in_flight_fences_[i]);
		}
	}

	void Swapchain::Init_Swapchain(Window* window)
	{
		vk::SurfaceKHR surface = window->GetVulkanSurface(device_->Get_Instance());
		VkHelpers::SwapChainProperties properties = VkHelpers::Query_SwapchainProperties(device_->Get_PhysicalDevice(), surface);

		vk::SurfaceFormatKHR surface_format = VkHelpers::Select_SwapchainFormat(properties.m_surfFormats);
		vk::PresentModeKHR present_mode = VkHelpers::Select_SwapchainPresentMode(properties.m_presentModes);
		swapchain_extent_ = VkHelpers::Select_SwapchainExtent(window, properties.m_surfCapabilities);
		swapchain_image_format_ = surface_format.format;

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
			.setImageFormat(swapchain_image_format_)
			.setImageColorSpace(surface_format.colorSpace)
			.setImageExtent(swapchain_extent_)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setPresentMode(present_mode)
			.setClipped(true)
			.setPreTransform(preTransform)
			.setCompositeAlpha(compositeAlpha)
			.setOldSwapchain(swapchain_);

		if (!device_->IsDifferentPresentQueue())
		{
			swapchain_create_info.setImageSharingMode(vk::SharingMode::eExclusive);
		}
		else
		{
			uint32_t graphics_family_queue = device_->GetQueueFamilyIndices().m_graphicsFamily.value();
			uint32_t presentation_family_queue = device_->GetQueueFamilyIndices().m_presentFamily.value();
			std::vector<uint32_t> queue_family_indices{ graphics_family_queue, presentation_family_queue };
			swapchain_create_info
				.setImageSharingMode(vk::SharingMode::eConcurrent)
				.setQueueFamilyIndices(queue_family_indices);
		}

		 auto createSwapchainResult = device_->Get_VkDevice().createSwapchainKHR(swapchain_create_info);
		 if (createSwapchainResult.result != vk::Result::eSuccess)
		 {
			 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create SwapchainKHR! Result code: %s.", vk::to_string(createSwapchainResult.result).c_str());
			 exit(1);
		 }
		 vk::SwapchainKHR new_swapchain = createSwapchainResult.value;
		SDL_Log("Swapchain created");

		// If old swapchain is valid, destroy it. (It happens on swapchain recreation)
		if (swapchain_)
		{
			SDL_Log("Swapchain was recreated. Cleaning old swapchain.");
			Cleanup_Swapchain();
		}

		// Assign the new swapchain to the window
		swapchain_ = new_swapchain;
	}

	void Swapchain::Init_SwapchainResources()
	{
		// Acquire swapchain images and create ImageViews
		{
			auto swapchainImagesKHRResult = device_->Get_VkDevice().getSwapchainImagesKHR(swapchain_);
			std::vector<vk::Image> swapchain_images = swapchainImagesKHRResult.value;
			image_resources_.resize(swapchain_images.size());
			for (uint32_t i = 0; i < image_resources_.size(); i++)
			{
				image_resources_[i].m_image = swapchain_images[i];

				vk::ImageViewCreateInfo image_view_create_info{};
				image_view_create_info
					.setImage(swapchain_images[i])
					.setViewType(vk::ImageViewType::e2D)
					.setFormat(swapchain_image_format_)
					.setSubresourceRange(vk::ImageSubresourceRange{}
						.setAspectMask(vk::ImageAspectFlagBits::eColor)
						.setBaseMipLevel(0)
						.setLevelCount(1)
						.setLayerCount(1)
						.setBaseArrayLayer(0)
					);

				 auto createImgViewResult = device_->Get_VkDevice().createImageView(image_view_create_info);
				 if (createImgViewResult.result != vk::Result::eSuccess)
				 {
					 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create ImageView for swapchain! Result code: %s.", vk::to_string(createImgViewResult.result).c_str());
					 exit(1);
				 }
				 image_resources_[i].m_image_view = createImgViewResult.value;
			}
		}

		SDL_Log("Images Resources initialized.");
	}

	void Swapchain::Init_RenderPass()
	{
		vk::AttachmentDescription color_attachment{};
		color_attachment
			.setFormat(swapchain_image_format_)
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

		vk::SubpassDependency subpass_dependency{};
		subpass_dependency
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlagBits::eNone)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead)
			.setDependencyFlags(vk::DependencyFlags());

		vk::RenderPassCreateInfo render_pass_info{};
		render_pass_info
			.setAttachments(color_attachment)
			.setSubpasses(subpass_description)
			.setDependencies(subpass_dependency);

		 auto createRenderPassResult = device_->Get_VkDevice().createRenderPass(render_pass_info);
		 if (createRenderPassResult.result != vk::Result::eSuccess)
		 {
			 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create RenderPass for swapchain! Result code: %s.", vk::to_string(createRenderPassResult.result).c_str());
			 exit(1);
		 }
		 render_pass_ = createRenderPassResult.value;

		SDL_Log("Render Pass Created");
	}

	void Swapchain::Init_Framebuffers()
	{
		for (size_t i = 0; i < image_resources_.size(); ++i)
		{
			vk::FramebufferCreateInfo framebuffer_info;
			framebuffer_info
				.setAttachments(image_resources_[i].m_image_view)
				.setRenderPass(render_pass_)
				.setWidth(swapchain_extent_.width)
				.setHeight(swapchain_extent_.height)
				.setLayers(1);

			 auto createFramebufferResult = device_->Get_VkDevice().createFramebuffer(framebuffer_info);
			 if (createFramebufferResult.result != vk::Result::eSuccess)
			 {
				 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create Framebuffer for swapchain! Result code: %s.", vk::to_string(createFramebufferResult.result).c_str());
				 exit(1);
			 }
			 image_resources_[i].m_framebuffer = createFramebufferResult.value;
			SDL_Log("Created Framebuffer for swapchain image number %d", i);
		}
	}

	void Swapchain::Init_Synchronization()
	{
		for (int i = 0; i < FRAME_LAG; i++)
		{
			 auto createImgAvailableSemaphoreResult = device_->Get_VkDevice().createSemaphore(vk::SemaphoreCreateInfo{});
			 if (createImgAvailableSemaphoreResult.result != vk::Result::eSuccess)
			 {
				 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create Image Available Semaphore for swapchain! Result code: %s.", vk::to_string(createImgAvailableSemaphoreResult.result).c_str());
				 exit(1);
			 }
			 image_available_semaphores_[i] = createImgAvailableSemaphoreResult.value;

			 auto createRenderFinishedSempahoreResult= device_->Get_VkDevice().createSemaphore(vk::SemaphoreCreateInfo{});
			 if (createRenderFinishedSempahoreResult.result != vk::Result::eSuccess)
			 {
				 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create Render Finished Semaphore for swapchain! Result code: %s.", vk::to_string(createRenderFinishedSempahoreResult.result).c_str());
				 exit(1);
			 }
			 render_finished_semaphores_[i] = createRenderFinishedSempahoreResult.value;

			 auto createInFlightFenceResult = device_->Get_VkDevice().createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
			 if (createInFlightFenceResult.result != vk::Result::eSuccess)
			 {
				 SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create In Flight Fence for swapchain! Result code: %s.", vk::to_string(createInFlightFenceResult.result).c_str());
				 exit(1);
			 }
			 in_flight_fences_[i] = createInFlightFenceResult.value;
		}

		SDL_Log("Created Swapchain synchronization semaphores and fences");
	}

	void Swapchain::Recreate_Swapchain()
	{
		device_->WaitIdle();

		Init_Swapchain(window_);
		Init_SwapchainResources();
		Init_Framebuffers();
	}

	void Swapchain::Cleanup_Swapchain()
	{
		for (int i = 0; i < image_resources_.size(); ++i)
		{
			ImageResources& resource = image_resources_[i];
			if (resource.m_framebuffer)
			{
				device_->Get_VkDevice().destroyFramebuffer(resource.m_framebuffer);
				resource.m_framebuffer = VK_NULL_HANDLE;
				SDL_Log("Framebuffer of Swapchain Image %d Destroyed.", i);
			}
			if (resource.m_image_view)
			{
				device_->Get_VkDevice().destroyImageView(resource.m_image_view);
				resource.m_image_view = VK_NULL_HANDLE;
				SDL_Log("ImageView of Swapchain Image %d Destroyed.", i);
			}
		}
		if (swapchain_)
		{
			device_->Get_VkDevice().destroySwapchainKHR(swapchain_);
			swapchain_ = VK_NULL_HANDLE;
			SDL_Log("Swapchain Destroyed.");
		}
	}
}
