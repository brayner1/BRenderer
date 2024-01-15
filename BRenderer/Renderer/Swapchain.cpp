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
		vk::PresentModeKHR present_mode     = VkHelpers::Select_SwapchainPresentMode(properties.m_presentModes, {vk::PresentModeKHR::eFifo});
		m_swapchain_extent                  = VkHelpers::Select_SwapchainExtent(window, properties.m_surfCapabilities);
		m_swapchain_image_format            = surface_format.format;

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
        m_swapchain_depth_format = VkHelpers::Select_SupportedFormat(m_render_device->Get_VkPhysicalDevice(),
                                                                     {
                                                                         vk::Format::eD32Sfloat,
                                                                         vk::Format::eD32SfloatS8Uint,
                                                                         vk::Format::eD24UnormS8Uint
                                                                     }, vk::ImageTiling::eOptimal,
                                                                     vk::FormatFeatureFlagBits::eDepthStencilAttachment);
		// Acquire swapchain images and create ImageViews
		auto swapchainImagesKHRResult = m_render_device->Get_VkDevice().getSwapchainImagesKHR(m_swapchain);
		std::vector<vk::Image> swapchain_images = swapchainImagesKHRResult.value;
		m_image_resources.resize(swapchain_images.size());

		for (uint32_t i = 0; i < m_image_resources.size(); i++)
		{
			// Create swapchain image ImageView
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
			// Create depth Image and ImageView
			{
			    vk::ImageCreateInfo img_create_info;
				img_create_info
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                    .setExtent(vk::Extent3D(m_swapchain_extent.width, m_swapchain_extent.height, 1))
                    .setFormat(m_swapchain_depth_format)
                    .setImageType(vk::ImageType::e2D)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setSharingMode(vk::SharingMode::eExclusive)
                    .setMipLevels(1)
                    .setArrayLayers(1);

				VmaAllocationCreateInfo alloc_create_info;
				alloc_create_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
				alloc_create_info.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
				alloc_create_info.requiredFlags = 0;
				alloc_create_info.preferredFlags = 0;
				alloc_create_info.memoryTypeBits = 0;
				alloc_create_info.pool = VK_NULL_HANDLE;
				alloc_create_info.pUserData = nullptr;
				alloc_create_info.priority = 1.0;

				VkImage new_depth_image;
				VmaAllocation depth_image_allocation;
				VmaAllocationInfo allocation_info;
				const vk::Result createImageResult = vk::Result(vmaCreateImage(m_render_device->Get_VmaAllocator(),
                                                                               reinterpret_cast<VkImageCreateInfo*>
                                                                               (&img_create_info),
                                                                               &alloc_create_info, &new_depth_image,
                                                                               &depth_image_allocation, &allocation_info));

				if (createImageResult != vk::Result::eSuccess)
				{
				    BRR_LogError("Could not create DepthBuffer Image for swapchain resources! Index {}. Result code: {}.", i, vk::to_string(createImageResult).c_str());
					exit(1);
				}

				vk::ImageViewCreateInfo image_view_create_info{};
			    image_view_create_info
                    .setImage(new_depth_image)
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(m_swapchain_depth_format)
                    .setSubresourceRange(vk::ImageSubresourceRange{}
                        .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                        .setBaseMipLevel(0)
                        .setLevelCount(1)
                        .setLayerCount(1)
                        .setBaseArrayLayer(0)
                    );

			    auto createImgViewResult = m_render_device->Get_VkDevice().createImageView(image_view_create_info);
			    if (createImgViewResult.result != vk::Result::eSuccess)
			    {
			        BRR_LogError("Could not create DepthBuffer ImageView for swapchain resources! Index {}. Result code: {}.", i, vk::to_string(createImgViewResult.result).c_str());
			        exit(1);
			    }
				m_image_resources[i].m_depth_image = new_depth_image;
			    m_image_resources[i].m_depth_image_view = createImgViewResult.value;
			    m_image_resources[i].m_depth_image_allocation = depth_image_allocation;
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

		vk::AttachmentDescription depth_attachment{};
		depth_attachment
			.setFormat(m_swapchain_depth_format)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::AttachmentReference color_attachment_ref{};
		color_attachment_ref
			.setAttachment(0)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

		vk::AttachmentReference depth_attachment_ref{};
		depth_attachment_ref
			.setAttachment(1)
			.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::SubpassDescription subpass_description{};
		subpass_description
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachments(color_attachment_ref)
	        .setPDepthStencilAttachment(&depth_attachment_ref);

		vk::SubpassDependency subpass_dependency[1];
		subpass_dependency[0]
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
			.setSrcAccessMask(vk::AccessFlagBits::eNone)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite/* | vk::AccessFlagBits::eColorAttachmentRead*/ | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
			.setDependencyFlags(vk::DependencyFlags());
		//subpass_dependency[1]
		//	.setSrcSubpass(0)
		//	.setDstSubpass(0)
		//	.setSrcStageMask(vk::PipelineStageFlagBits::eTopOfPipe)
		//	.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite/* | vk::AccessFlagBits::eColorAttachmentRead*/)
		//	.setDstStageMask(vk::PipelineStageFlagBits::eVertexInput)
		//	.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
		//	.setDependencyFlags(vk::DependencyFlags());

		std::array<vk::AttachmentDescription, 2> attachments {color_attachment, depth_attachment};
		vk::RenderPassCreateInfo render_pass_info{};
		render_pass_info
			.setAttachments(attachments)
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
			std::array<vk::ImageView, 2> attachments {m_image_resources[i].m_image_view, m_image_resources[i].m_depth_image_view};
			vk::FramebufferCreateInfo framebuffer_info;
			framebuffer_info
				.setAttachments(attachments)
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
			if (resource.m_depth_image_view)
			{
			    m_render_device->Get_VkDevice().destroyImageView(resource.m_depth_image_view);
				resource.m_depth_image_view = VK_NULL_HANDLE;
				BRR_LogInfo("ImageView of Swapchain's DepthBuffer Image {} Destroyed.", i);
			}
			if (resource.m_depth_image)
			{
			    vmaDestroyImage(m_render_device->Get_VmaAllocator(), resource.m_depth_image, resource.m_depth_image_allocation);
				resource.m_depth_image = VK_NULL_HANDLE;
				resource.m_depth_image_allocation = VK_NULL_HANDLE;
				BRR_LogInfo("Image of Swapchain's DepthBuffer Image {} destroyed.", i);
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
