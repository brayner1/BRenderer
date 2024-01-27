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
		Init_Synchronization();
	}

	vk::Result Swapchain::AcquireNextImage(vk::Semaphore& image_available_semaphore)
	{
		if (m_render_device->m_device.waitForFences(m_in_flight_fences[m_current_buffer_idx], true, UINT64_MAX) != vk::Result::eSuccess)
		{
			BRR_LogError("Error waiting for In Flight Fence");
			exit(1);
		}

		m_render_device->m_device.resetFences(m_in_flight_fences[m_current_buffer_idx]);

		vk::Result result = m_render_device->m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX,
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

		vk::Result result = m_render_device->m_presentation_queue.presentKHR(present_info);
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

    void Swapchain::BeginRendering()
    {
		vk::CommandBuffer command_buffer = m_render_device->GetCurrentGraphicsCommandBuffer();
		// Image transition from Undefined to Color Attachment
	    {
            m_render_device->TransitionImageLayout(command_buffer, m_image_resources[m_current_image_idx].m_image,
                                                   vk::ImageLayout::eUndefined,
                                                   vk::ImageLayout::eColorAttachmentOptimal,
                                                   vk::AccessFlagBits2::eMemoryWrite,
                                                   vk::PipelineStageFlagBits2::eAllCommands,
                                                   vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                                   vk::PipelineStageFlagBits2::eAllCommands,
                                                   vk::ImageAspectFlagBits::eColor);
	    }

		// DepthImage transition from Undefined to Depth Attachment
	    {
			m_render_device->TransitionImageLayout(command_buffer, m_image_resources[m_current_image_idx].m_depth_image,
                                                   vk::ImageLayout::eUndefined,
                                                   vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                                   vk::AccessFlagBits2::eMemoryWrite,
                                                   vk::PipelineStageFlagBits2::eAllCommands,
                                                   vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                                   vk::PipelineStageFlagBits2::eAllCommands,
                                                   vk::ImageAspectFlagBits::eDepth);
	    }

		std::array<vk::ClearValue, 2> clear_values { vk::ClearColorValue {0.2f, 0.2f, 0.2f, 1.f}, vk::ClearDepthStencilValue {1.0, 0} };
		ImageResources& image_resource = m_image_resources[m_current_image_idx];

        vk::Viewport viewport{
            0, 0,
            static_cast<float>(m_swapchain_extent.width), static_cast<float>(m_swapchain_extent.height), 0.0, 1.0
        };
		vk::Rect2D scissor {{0, 0}, m_swapchain_extent};

		command_buffer.setViewport(0, viewport);
		command_buffer.setScissor(0, scissor);

		vk::RenderingAttachmentInfo color_attachment_info {};
		color_attachment_info
		    .setClearValue(clear_values[0])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setImageView(image_resource.m_image_view)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore);

		vk::RenderingAttachmentInfo depth_attachment_info {};
		depth_attachment_info
		    .setClearValue(clear_values[1])
            .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setImageView(image_resource.m_depth_image_view)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore);

		vk::RenderingInfo rendering_info {};
		rendering_info
            .setColorAttachments(color_attachment_info)
		    .setPDepthAttachment(&depth_attachment_info)
            .setLayerCount(1)
            .setRenderArea(scissor);

		command_buffer.beginRendering(rendering_info);
    }

    void Swapchain::EndRendering()
    {
		vk::CommandBuffer command_buffer = m_render_device->GetCurrentGraphicsCommandBuffer();
		command_buffer.endRendering();

		// Image transition from Color Attachment to Present Src
	    {
			m_render_device->TransitionImageLayout(command_buffer, m_image_resources[m_current_image_idx].m_image,
                                                   vk::ImageLayout::eColorAttachmentOptimal,
                                                   vk::ImageLayout::ePresentSrcKHR,
                                                   vk::AccessFlagBits2::eMemoryWrite,
                                                   vk::PipelineStageFlagBits2::eAllCommands,
                                                   vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                                   vk::PipelineStageFlagBits2::eAllCommands,
                                                   vk::ImageAspectFlagBits::eColor);
	    }
    }

    Swapchain::~Swapchain()
	{
		Cleanup_Swapchain();

		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			m_render_device->m_device.destroySemaphore(m_image_available_semaphores[i]);
			m_render_device->m_device.destroyFence(m_in_flight_fences[i]);
		}
	}

	void Swapchain::Init_Swapchain(vis::Window* window)
	{
		vk::SurfaceKHR surface = window->GetVulkanSurface(m_render_device->m_vulkan_instance);
		VkHelpers::SwapChainProperties properties = VkHelpers::Query_SwapchainProperties(m_render_device->m_phys_device, surface);

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

		 auto createSwapchainResult = m_render_device->m_device.createSwapchainKHR(swapchain_create_info);
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
        m_swapchain_depth_format = VkHelpers::Select_SupportedFormat(m_render_device->m_phys_device,
                                                                     {
                                                                         vk::Format::eD32Sfloat,
                                                                         vk::Format::eD32SfloatS8Uint,
                                                                         vk::Format::eD24UnormS8Uint
                                                                     }, vk::ImageTiling::eOptimal,
                                                                     vk::FormatFeatureFlagBits::eDepthStencilAttachment);
		// Acquire swapchain images and create ImageViews
		auto swapchainImagesKHRResult = m_render_device->m_device.getSwapchainImagesKHR(m_swapchain);
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

			    auto createImgViewResult = m_render_device->m_device.createImageView(image_view_create_info);
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
				const vk::Result createImageResult = vk::Result(vmaCreateImage(m_render_device->m_vma_allocator,
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

			    auto createImgViewResult = m_render_device->m_device.createImageView(image_view_create_info);
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

	void Swapchain::Init_Synchronization()
	{
		for (int i = 0; i < FRAME_LAG; i++)
		{
			 auto createImgAvailableSemaphoreResult = m_render_device->m_device.createSemaphore(vk::SemaphoreCreateInfo{});
			 if (createImgAvailableSemaphoreResult.result != vk::Result::eSuccess)
			 {
				 BRR_LogError("Could not create Image Available Semaphore for swapchain! Result code: {}.", vk::to_string(createImgAvailableSemaphoreResult.result).c_str());
				 exit(1);
			 }
			 m_image_available_semaphores[i] = createImgAvailableSemaphoreResult.value;

			 auto createInFlightFenceResult = m_render_device->m_device.createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
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
	}

	void Swapchain::Cleanup_Swapchain()
	{
		for (int i = 0; i < m_image_resources.size(); ++i)
		{
			ImageResources& resource = m_image_resources[i];
			if (resource.m_image_view)
			{
				m_render_device->m_device.destroyImageView(resource.m_image_view);
				resource.m_image_view = VK_NULL_HANDLE;
				BRR_LogInfo("ImageView of Swapchain Image {} Destroyed.", i);
			}
			if (resource.m_depth_image_view)
			{
			    m_render_device->m_device.destroyImageView(resource.m_depth_image_view);
				resource.m_depth_image_view = VK_NULL_HANDLE;
				BRR_LogInfo("ImageView of Swapchain's DepthBuffer Image {} Destroyed.", i);
			}
			if (resource.m_depth_image)
			{
			    vmaDestroyImage(m_render_device->m_vma_allocator, resource.m_depth_image, resource.m_depth_image_allocation);
				resource.m_depth_image = VK_NULL_HANDLE;
				resource.m_depth_image_allocation = VK_NULL_HANDLE;
				BRR_LogInfo("Image of Swapchain's DepthBuffer Image {} destroyed.", i);
			}
		}
		if (m_swapchain)
		{
			m_render_device->m_device.destroySwapchainKHR(m_swapchain);
			m_swapchain = VK_NULL_HANDLE;
			BRR_LogInfo("Swapchain Destroyed.");
		}
	}
}
