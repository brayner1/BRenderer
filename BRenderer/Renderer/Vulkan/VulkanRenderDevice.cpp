#include "VulkanRenderDevice.h"

#include <Renderer/RenderDefs.h>
#include <Renderer/Descriptors.h>
#include <Renderer/Vulkan/VKDescriptors.h>

#include <Visualization/Window.h>
#include <Core/LogSystem.h>
#include <Files/FilesUtils.h>
#include <Geometry/Geometry.h>

#include <filesystem>
#include <iostream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
static vk::DynamicLoader VulkanDynamicLoader;

namespace brr::render
{
    static vk::BufferUsageFlags VkBufferUsageFromDeviceBufferUsage(BufferUsage buffer_usage)
    {
        vk::BufferUsageFlags output {};
        if (buffer_usage & BufferUsage::TransferSrc)
        {
            output |= vk::BufferUsageFlagBits::eTransferSrc;
        }
        if (buffer_usage & BufferUsage::TransferDst)
        {
            output |= vk::BufferUsageFlagBits::eTransferDst;
        }
        if (buffer_usage & BufferUsage::UniformTexelBuffer)
        {
            output |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
        }
        if (buffer_usage & BufferUsage::StorageTexelBuffer)
        {
            output |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
        }
        if (buffer_usage & BufferUsage::UniformBuffer)
        {
            output |= vk::BufferUsageFlagBits::eUniformBuffer;
        }
        if (buffer_usage & BufferUsage::StorageBuffer)
        {
            output |= vk::BufferUsageFlagBits::eStorageBuffer;
        }
        //if (buffer_usage & BufferUsage::IndexBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eIndexBuffer;
        //}
        //if (buffer_usage & BufferUsage::VertexBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eVertexBuffer;
        //}
        //if (buffer_usage & BufferUsage::IndirectBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eIndirectBuffer;
        //}
        //if (buffer_usage & BufferUsage::ShaderDeviceAddress)
        //{
        //	output |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
        //}
        //if (buffer_usage & BufferUsage::VideoDecodeSrc)
        //{
        //	output |= vk::BufferUsageFlagBits::eVideoDecodeSrcKHR;
        //}
        //if (buffer_usage & BufferUsage::VideoDecodeDst)
        //{
        //	output |= vk::BufferUsageFlagBits::eVideoDecodeDstKHR;
        //}
        //if (buffer_usage & BufferUsage::TransformFeedbackBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;
        //}
        //if (buffer_usage & BufferUsage::TransformFeedbackCounterBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eTransformFeedbackCounterBufferEXT;
        //}
        //if (buffer_usage & BufferUsage::ConditionalRendering)
        //{
        //	output |= vk::BufferUsageFlagBits::eConditionalRenderingEXT;
        //}
        //if (buffer_usage & BufferUsage::AccelerationStructureBuildInputReadOnly)
        //{
        //	output |= vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
        //}
        //if (buffer_usage & BufferUsage::AccelerationStructureStorage)
        //{
        //	output |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
        //}
        //if (buffer_usage & BufferUsage::ShaderBindingTable)
        //{
        //	output |= vk::BufferUsageFlagBits::eShaderBindingTableKHR;
        //}
        //if (buffer_usage & BufferUsage::RayTracingNV)
        //{
        //	output |= vk::BufferUsageFlagBits::eRayTracingNV;
        //}
        return output;
    }

    static vk::ImageUsageFlags VkImageUsageFromDeviceImageUsage(ImageUsage image_usage)
    {
        vk::ImageUsageFlags result = {};
        if (image_usage & ImageUsage::TransferSrcImage)
        {
            result |= vk::ImageUsageFlagBits::eTransferSrc;
        }
        if (image_usage & ImageUsage::TransferDstImage)
        {
            result |= vk::ImageUsageFlagBits::eTransferDst;
        }
        if (image_usage & ImageUsage::SampledImage)
        {
            result |= vk::ImageUsageFlagBits::eSampled;
        }
        if (image_usage & ImageUsage::StorageImage)
        {
            result |= vk::ImageUsageFlagBits::eStorage;
        }
        if (image_usage & ImageUsage::ColorAttachmentImage)
        {
            result |= vk::ImageUsageFlagBits::eColorAttachment;
        }
        if (image_usage & ImageUsage::DepthStencilAttachmentImage)
        {
            result |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        }
        if (image_usage & ImageUsage::TransientAttachmentImage)
        {
            result |= vk::ImageUsageFlagBits::eTransientAttachment;
        }
        if (image_usage & ImageUsage::InputAttachmentImage)
        {
            result |= vk::ImageUsageFlagBits::eInputAttachment;
        }
        return result;
    }

    VmaMemoryUsage VmaMemoryUsageFromDeviceMemoryUsage(MemoryUsage memory_usage)
    {
        switch (memory_usage)
        {
        case MemoryUsage::AUTO:
            return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
        case MemoryUsage::AUTO_PREFER_DEVICE:
            return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        case MemoryUsage::AUTO_PREFER_HOST:
            return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        default:
            return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
        }
    }

    std::unique_ptr<VulkanRenderDevice> VulkanRenderDevice::device_instance {};

    void VulkanRenderDevice::CreateRenderDevice(vis::Window* window)
    {
        assert(!device_instance && "VulkanRenderDevice is already created. You can only create one.");
        device_instance.reset(new VulkanRenderDevice(window));
    }

    void VulkanRenderDevice::DestroyRenderDevice()
    {
        device_instance.reset();
    }

    VulkanRenderDevice* VulkanRenderDevice::GetSingleton()
    {
        assert(device_instance && "Can't get non-initialized VulkanRenderDevice. Run `VKRD::CreateRenderDevice(Window* window)` before this function.");
        return device_instance.get();
    }

    VulkanRenderDevice::VulkanRenderDevice(vis::Window* main_window)
    {
        BRR_LogInfo("Constructing VulkanRenderDevice");
        Init_VkInstance(main_window);
        vk::SurfaceKHR surface = main_window->GetVulkanSurface(m_vulkan_instance);
        Init_PhysDevice(surface);
        Init_Queues_Indices(surface);
        Init_Device();
        Init_Allocator();
        Init_CommandPool();
        Init_Frames();

        m_staging_allocator.Init(this);

        m_descriptor_layout_cache.reset(new DescriptorLayoutCache(m_device));
        m_descriptor_allocator.reset(new DescriptorSetAllocator(m_device));

        Init_Texture2DSampler();

        BRR_LogInfo("VulkanRenderDevice {:#x} constructed", (size_t)this);
    }

    VulkanRenderDevice::~VulkanRenderDevice()
    {
        BRR_LogDebug("Starting VulkanRenderDevice destruction.");

        BRR_LogTrace("Waiting device idle.");
        WaitIdle();
        BRR_LogTrace("Device idle. Starting destroy process");

        for (size_t idx = 0; idx < FRAME_LAG; ++idx)
        {
            Free_FramePendingResources(m_frames[idx]);
            m_device.destroySemaphore(m_frames[idx].render_finished_semaphore);
            m_device.destroySemaphore(m_frames[idx].transfer_finished_semaphore);
            m_device.destroyFence(m_frames[idx].in_flight_fences);
        }
        BRR_LogTrace("Destroyed frame sync semaphores.");

        m_staging_allocator.DestroyAllocator();
        BRR_LogTrace("Destroyed staging allocator.");

        vmaDestroyAllocator(m_vma_allocator);
        BRR_LogTrace("Destroyed VMA allocator.");

        m_device.destroySampler(m_texture2DSampler);
        m_texture2DSampler = VK_NULL_HANDLE;

        m_descriptor_layout_cache.reset();
        BRR_LogTrace("Destroyed descriptor layout cache.");
        m_descriptor_allocator.reset();
        BRR_LogTrace("Destroyed descriptor allocator.");

        if (m_graphics_command_pool)
        {
            m_device.destroyCommandPool(m_graphics_command_pool);
            m_graphics_command_pool = VK_NULL_HANDLE;
            BRR_LogTrace("Destroyed graphics command pool.");
        }
        if (m_present_command_pool)
        {
            m_device.destroyCommandPool(m_present_command_pool);
            m_present_command_pool = VK_NULL_HANDLE;
            BRR_LogTrace("Destroyed present command pool.");
        }
        if (m_transfer_command_pool)
        {
            m_device.destroyCommandPool(m_transfer_command_pool);
            m_transfer_command_pool = VK_NULL_HANDLE;
            BRR_LogTrace("Destroyed transfer command pool.");
        }

        m_device.destroy();
        BRR_LogTrace("Destroyed Vulkan device.");

        BRR_LogInfo("VulkanRenderDevice destroyed.");
    }

    uint32_t VulkanRenderDevice::BeginFrame()
    {
        Frame& current_frame = GetCurrentFrame();
        if (current_frame.frame_in_progress)
        {
            BRR_LogDebug("Frame is already in progress. Aborting BeginFrame.");
            return m_current_frame;
        }

        if (m_device.waitForFences(current_frame.in_flight_fences, true, UINT64_MAX) != vk::Result::eSuccess)
        {
            BRR_LogError("Error waiting for In Flight Fence");
            exit(1);
        }

        m_device.resetFences(current_frame.in_flight_fences);

        Free_FramePendingResources(current_frame);

        if (!current_frame.graphics_cmd_buffer_begin)
        {
            vk::Result graph_begin_result = BeginCommandBuffer(current_frame.graphics_cmd_buffer);
            current_frame.graphics_cmd_buffer_begin = true;
        }

        if (!current_frame.transfer_cmd_buffer_begin)
        {
            vk::Result transf_begin_result = BeginCommandBuffer(current_frame.transfer_cmd_buffer);
            current_frame.transfer_cmd_buffer_begin = true;
        }
        current_frame.frame_in_progress = true;

        BRR_LogTrace("Begin frame {}", m_current_frame);

        return m_current_frame;
    }

    void VulkanRenderDevice::EndFrame()
    {
        Frame& current_frame = GetCurrentFrame();

        current_frame.graphics_cmd_buffer.end();
        current_frame.transfer_cmd_buffer.end();

        current_frame.graphics_cmd_buffer_begin = false;
        current_frame.transfer_cmd_buffer_begin = false;

        vk::Result transfer_result = SubmitTransferCommandBuffers(1, &current_frame.transfer_cmd_buffer, 0, nullptr,
                                                                  nullptr, 1,
                                                                  &current_frame.transfer_finished_semaphore, nullptr);
        BRR_LogTrace("Transfer command buffer submitted. Buffer: {:#x}. Frame {}. Buffer Index: {}",
                     size_t(VkCommandBuffer((current_frame.transfer_cmd_buffer))), m_current_frame, m_current_buffer);

        std::vector<vk::Semaphore>& wait_semaphores = current_frame.images_available_semaphore;
        wait_semaphores.push_back(current_frame.transfer_finished_semaphore);
        std::vector<vk::PipelineStageFlags> wait_stages (wait_semaphores.size(), vk::PipelineStageFlagBits::eColorAttachmentOutput);
        wait_stages.back() = vk::PipelineStageFlagBits::eVertexInput;

        bool will_present = current_frame.swapchain_present_infos.size() > 0;

        vk::Result result = SubmitGraphicsCommandBuffers(1, &current_frame.graphics_cmd_buffer, wait_semaphores.size(),
                                                         wait_semaphores.data(), wait_stages.data(), will_present? 1 : 0,
                                                         &current_frame.render_finished_semaphore,
                                                         current_frame.in_flight_fences);
        BRR_LogTrace("Graphics command buffer submitted. Buffer: {:#x}. Frame {}. Buffer Index: {}",
                     size_t(VkCommandBuffer(current_frame.graphics_cmd_buffer)), m_current_frame, m_current_buffer);

        BRR_LogTrace("Frame ended. Frame: {}. Buffer: {}", m_current_frame, m_current_buffer);

        for (uint32_t idx = 0; idx < current_frame.swapchain_present_infos.size(); idx++)
        {
            Frame::SwapchainPresent& swapchain_present = current_frame.swapchain_present_infos[idx];
            swapchain_present.present_info.setWaitSemaphores(current_frame.render_finished_semaphore);

            vk::Result result = m_presentation_queue.presentKHR(swapchain_present.present_info);
            if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
            {
                Swapchain_Recreate(swapchain_present.swapchain_handle);
            }
            else if (result != vk::Result::eSuccess)
            {
                BRR_LogError("Presentation Failed.");
            }
        }
        current_frame.swapchain_present_infos.clear();
        current_frame.images_available_semaphore.clear();
        current_frame.frame_in_progress = false;

        ++m_current_frame;
        m_current_buffer = (m_current_frame % FRAME_LAG);
    }

    void VulkanRenderDevice::WaitIdle() const
    {
        BRR_LogTrace("Waiting for device idle.");
        m_device.waitIdle();
    }

    /*************
     * Swapchain *
     *************/

    SwapchainHandle VulkanRenderDevice::Swapchain_Create(vis::Window* window)
    {
        if (!window)
        {
            BRR_LogError("Can't initialize Swapchain with NULL Window.");
            return null_handle;
        }

        ResourceHandle swapchain_handle = m_swapchain_alloc.CreateResource();
        Swapchain* swapchain = m_swapchain_alloc.GetResource(swapchain_handle);

        swapchain->window = window;

        Init_Swapchain(*swapchain);
        Init_SwapchainResources(*swapchain);
        Init_SwapchainSynchronization(*swapchain);

        return swapchain_handle;
    }

    void VulkanRenderDevice::Swapchain_Recreate(SwapchainHandle swapchain_handle)
    {
        if (!m_swapchain_alloc.OwnsResource(swapchain_handle))
        {
            BRR_LogError("Can't recreate Swapchain that does not exist.");
            return;
        }
        BRR_LogInfo("Recreating Swapchain");
        Swapchain* swapchain = m_swapchain_alloc.GetResource(swapchain_handle);
        WaitIdle();

        Init_Swapchain(*swapchain);
        Init_SwapchainResources(*swapchain);
    }

    void VulkanRenderDevice::Swapchain_Destroy(SwapchainHandle swapchain_handle)
    {
        if (!m_swapchain_alloc.OwnsResource(swapchain_handle))
        {
            BRR_LogError("Can't destroy Swapchain that does not exist.");
            return;
        }
        Swapchain* swapchain = m_swapchain_alloc.GetResource(swapchain_handle);

        Cleanup_Swapchain(*swapchain);

        for (uint32_t idx = 0; idx < FRAME_LAG; idx++)
        {
            if (swapchain->image_available_semaphores[idx])
            {
                m_device.destroySemaphore(swapchain->image_available_semaphores[idx]);
                swapchain->image_available_semaphores[idx] = VK_NULL_HANDLE;
            }
        }

        m_swapchain_alloc.DestroyResource(swapchain_handle);
    }

    uint32_t VulkanRenderDevice::Swapchain_AcquireNextImage(SwapchainHandle swapchain_handle)
    {
        if (!m_swapchain_alloc.OwnsResource(swapchain_handle))
        {
            BRR_LogError("Can't acquire image of Swapchain that does not exist.");
            return -1;
        }
        Swapchain* swapchain = m_swapchain_alloc.GetResource(swapchain_handle);
        Frame& current_frame = GetCurrentFrame();

        vk::Result result = m_device.acquireNextImageKHR(swapchain->swapchain, UINT64_MAX,
                swapchain->image_available_semaphores[swapchain->current_buffer_idx], VK_NULL_HANDLE, &swapchain->current_image_idx);

        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            Swapchain_Recreate(swapchain_handle);
            result = m_device.acquireNextImageKHR(swapchain->swapchain, UINT64_MAX,
                swapchain->image_available_semaphores[swapchain->current_buffer_idx], VK_NULL_HANDLE, &swapchain->current_image_idx);
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            //throw std::runtime_error("Failed to acquire DeviceSwapchain image!");
            return false;
        }

        current_frame.images_available_semaphore.push_back(swapchain->image_available_semaphores[swapchain->current_buffer_idx]);

        return swapchain->current_image_idx;
    }

    bool VulkanRenderDevice::Swapchain_PresentCurrentImage(SwapchainHandle swapchain_handle)
    {
        if (!m_swapchain_alloc.OwnsResource(swapchain_handle))
        {
            BRR_LogError("Can't present image of Swapchain that does not exist.");
            return false;
        }
        Swapchain* swapchain = m_swapchain_alloc.GetResource(swapchain_handle);
        Frame& current_frame = GetCurrentFrame();

        vk::PresentInfoKHR present_info{};
        present_info
            .setSwapchains(swapchain->swapchain)
            .setImageIndices(swapchain->current_image_idx);

        current_frame.swapchain_present_infos.emplace_back(present_info, swapchain_handle);

        swapchain->current_buffer_idx = (swapchain->current_buffer_idx + 1) % FRAME_LAG;

        return true;
    }

    std::vector<Texture2DHandle> VulkanRenderDevice::GetSwapchainImages(SwapchainHandle swapchain_handle)
    {
        if (!m_swapchain_alloc.OwnsResource(swapchain_handle))
        {
            BRR_LogError("Can't get swapchain images. Provided SwapchainHandle does not exist.");
            return {};
        }
        Swapchain* swapchain = m_swapchain_alloc.GetResource(swapchain_handle);

        return swapchain->image_resources;
    }

    void VulkanRenderDevice::Swapchain_BeginRendering(SwapchainHandle swapchain_handle, Texture2DHandle depth_image_handle)
    {
        if (!m_swapchain_alloc.OwnsResource(swapchain_handle))
        {
            BRR_LogError("Can't begin rendering of Swapchain that does not exist.");
            return;
        }
        Swapchain* swapchain = m_swapchain_alloc.GetResource(swapchain_handle);
        Texture2D* swapchain_image = m_texture2d_alloc.GetResource(swapchain->image_resources[swapchain->current_image_idx]);
        Texture2D* depth_image = nullptr;
        if (m_texture2d_alloc.OwnsResource(depth_image_handle))
        {
            depth_image = m_texture2d_alloc.GetResource(depth_image_handle);
        }

        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();
        // Image transition from Undefined to Color Attachment
        {
            TransitionImageLayout(command_buffer, swapchain_image->image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::ImageAspectFlagBits::eColor);
        }

        // DepthImage transition from Undefined to Depth Attachment
        if (depth_image)
        {
            TransitionImageLayout(command_buffer, depth_image->image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::ImageAspectFlagBits::eDepth);
        }

        std::array<vk::ClearValue, 2> clear_values { vk::ClearColorValue {0.2f, 0.2f, 0.2f, 1.f}, vk::ClearDepthStencilValue {1.0, 0} };
        

        vk::Viewport viewport{
            0, 0,
            static_cast<float>(swapchain->swapchain_extent.width), static_cast<float>(swapchain->swapchain_extent.height), 0.0, 1.0
        };
        vk::Rect2D scissor {{0, 0}, swapchain->swapchain_extent};

        command_buffer.setViewport(0, viewport);
        command_buffer.setScissor(0, scissor);

        vk::RenderingAttachmentInfo color_attachment_info {};
        color_attachment_info
            .setClearValue(clear_values[0])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setImageView(swapchain_image->image_view)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore);

        vk::RenderingInfo rendering_info {};
        rendering_info
            .setColorAttachments(color_attachment_info)
            .setLayerCount(1)
            .setRenderArea(scissor);

        if (depth_image)
        {
            vk::RenderingAttachmentInfo depth_attachment_info {};
            depth_attachment_info
                .setClearValue(clear_values[1])
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setImageView(depth_image->image_view)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore);

            rendering_info.setPDepthAttachment(&depth_attachment_info);
        }

        command_buffer.beginRendering(rendering_info);
    }

    void VulkanRenderDevice::Swapchain_EndRendering(SwapchainHandle swapchain_handle)
    {
        if (!m_swapchain_alloc.OwnsResource(swapchain_handle))
        {
            BRR_LogError("Can't end rendering of Swapchain that does not exist.");
            return;
        }
        Swapchain* swapchain = m_swapchain_alloc.GetResource(swapchain_handle);

        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();
        command_buffer.endRendering();

        // Image transition from Color Attachment to Present Src
        {
            Texture2D* swapchain_image = m_texture2d_alloc.GetResource(swapchain->image_resources[swapchain->current_image_idx]);
            TransitionImageLayout(command_buffer, swapchain_image->image,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::ImageLayout::ePresentSrcKHR,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::ImageAspectFlagBits::eColor);
        }
    }

    void VulkanRenderDevice::RenderTarget_BeginRendering(Texture2DHandle color_attachment_handle,
                                                         Texture2DHandle depth_attachment_handle,
                                                         bool use_stencil)
    {
        if (!m_texture2d_alloc.OwnsResource(color_attachment_handle))
        {
            BRR_LogError("Can't begin rendering using color attachment that does not exist.");
            return;
        }
        if (!m_texture2d_alloc.OwnsResource(depth_attachment_handle))
        {
            BRR_LogError("Can't begin rendering using depth attachment that does not exist.");
            return;
        }

        Texture2D* color_attachment = m_texture2d_alloc.GetResource(color_attachment_handle);
        Texture2D* depth_attachment = m_texture2d_alloc.GetResource(depth_attachment_handle);

        if (color_attachment->image_extent != depth_attachment->image_extent)
        {
            BRR_LogError(
                "Can't begin rendering using color attachment and depth attachment of different sizes.\nColor Attachment: {}. Size: {}x{}\nDepth Attachment: {}. Size: {}x{}",
                (size_t)color_attachment, color_attachment->image_extent.width, color_attachment->image_extent.height, 
                (size_t)depth_attachment, depth_attachment->image_extent.width, depth_attachment->image_extent.height);
            return;
        }
        uint32_t width  = color_attachment->image_extent.width;
        uint32_t height = color_attachment->image_extent.height;

        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();
        // Image transition from Undefined to Color Attachment
        {
            TransitionImageLayout(command_buffer, color_attachment->image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::ImageAspectFlagBits::eColor);
        }

        // DepthImage transition from Undefined to Depth Attachment
        {
            TransitionImageLayout(command_buffer, depth_attachment->image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                  vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                                  vk::ImageAspectFlagBits::eDepth);
        }

        std::array<vk::ClearValue, 2> clear_values { vk::ClearColorValue {0.2f, 0.2f, 0.2f, 1.f}, vk::ClearDepthStencilValue {1.0, 0} };
        

        vk::Viewport viewport{
            0, 0,
            static_cast<float>(width), static_cast<float>(height), 0.0, 1.0
        };
        vk::Rect2D scissor {{0, 0}, color_attachment->image_extent};

        command_buffer.setViewport(0, viewport);
        command_buffer.setScissor(0, scissor);

        vk::RenderingAttachmentInfo color_attachment_info {};
        color_attachment_info
            .setClearValue(clear_values[0])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setImageView(color_attachment->image_view)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore);

        vk::RenderingAttachmentInfo depth_attachment_info {};
        depth_attachment_info
            .setClearValue(clear_values[1])
            .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setImageView(depth_attachment->image_view)
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

    void VulkanRenderDevice::RenderTarget_EndRendering(Texture2DHandle color_attachment_handle)
    {
        if (!m_texture2d_alloc.OwnsResource(color_attachment_handle))
        {
            BRR_LogError("Can't end rendering using color attachment that does not exist.");
            return;
        }
        Texture2D* color_attachment = m_texture2d_alloc.GetResource(color_attachment_handle);

        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();
        command_buffer.endRendering();

        // Image transition from Color Attachment to Present Src
        {
            TransitionImageLayout(command_buffer, color_attachment->image,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  color_attachment->image_layout,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::ImageAspectFlagBits::eColor);
        }
    }

    void VulkanRenderDevice::Init_Swapchain(Swapchain& swapchain)
    {
        vk::SurfaceKHR surface = swapchain.window->GetVulkanSurface(m_vulkan_instance);
        VkHelpers::SwapChainProperties properties = VkHelpers::Query_SwapchainProperties(m_phys_device, surface);

        vk::SurfaceFormatKHR surface_format = VkHelpers::Select_SwapchainFormat(properties.m_surfFormats);
        vk::PresentModeKHR present_mode     = VkHelpers::Select_SwapchainPresentMode(properties.m_presentModes, {vk::PresentModeKHR::eFifo});
        swapchain.swapchain_extent          = VkHelpers::Select_SwapchainExtent(swapchain.window, properties.m_surfCapabilities);
        swapchain.swapchain_image_format    = surface_format.format;

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
            .setImageFormat(swapchain.swapchain_image_format)
            .setImageColorSpace(surface_format.colorSpace)
            .setImageExtent(swapchain.swapchain_extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
            .setPresentMode(present_mode)
            .setClipped(true)
            .setPreTransform(preTransform)
            .setCompositeAlpha(compositeAlpha)
            .setOldSwapchain(swapchain.swapchain);

        if (!IsDifferentPresentQueue())
        {
            swapchain_create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        }
        else
        {
            uint32_t graphics_family_queue = GetQueueFamilyIndices().m_graphicsFamily.value();
            uint32_t presentation_family_queue = GetQueueFamilyIndices().m_presentFamily.value();
            std::vector<uint32_t> queue_family_indices{ graphics_family_queue, presentation_family_queue };
            swapchain_create_info
                .setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndices(queue_family_indices);
        }

        auto createSwapchainResult = m_device.createSwapchainKHR(swapchain_create_info);
        if (createSwapchainResult.result != vk::Result::eSuccess)
        {
         BRR_LogError("Could not create SwapchainKHR! Result code: {}.", vk::to_string(createSwapchainResult.result).c_str());
         exit(1);
        }
        vk::SwapchainKHR new_swapchain = createSwapchainResult.value;
        BRR_LogInfo("Swapchain created");

        // If old swapchain is valid, destroy it. (It happens on swapchain recreation)
        if (swapchain.swapchain)
        {
            BRR_LogInfo("Swapchain was recreated. Cleaning old swapchain.");
            Cleanup_Swapchain(swapchain);
        }

        // Assign the new swapchain to the window
        swapchain.swapchain = new_swapchain;
    }

    void VulkanRenderDevice::Init_SwapchainResources(Swapchain& swapchain)
    {
        // Acquire swapchain images and create ImageViews
        auto swapchainImagesKHRResult = m_device.getSwapchainImagesKHR(swapchain.swapchain);
        std::vector<vk::Image> swapchain_images = swapchainImagesKHRResult.value;
        swapchain.image_resources.resize(swapchain_images.size());

        for (uint32_t i = 0; i < swapchain.image_resources.size(); i++)
        {
            Texture2D* swapchain_image;
            swapchain.image_resources[i] = m_texture2d_alloc.CreateResource(&swapchain_image);
            swapchain_image->image_extent = swapchain.swapchain_extent;
            swapchain_image->image_format = swapchain.swapchain_image_format;
            swapchain_image->image_layout = vk::ImageLayout::ePresentSrcKHR;

            // Create swapchain image ImageView
            swapchain_image->image = swapchain_images[i];

            vk::ImageViewCreateInfo image_view_create_info{};
            image_view_create_info
                .setImage(swapchain_images[i])
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(swapchain.swapchain_image_format)
                .setSubresourceRange(vk::ImageSubresourceRange{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseMipLevel(0)
                    .setLevelCount(1)
                    .setLayerCount(1)
                    .setBaseArrayLayer(0)
                );

            auto createImgViewResult = m_device.createImageView(image_view_create_info);
            if (createImgViewResult.result != vk::Result::eSuccess)
            {
                BRR_LogError("Could not create ImageView for swapchain! Result code: {}.", vk::to_string(createImgViewResult.result).c_str());
                exit(1);
            }
            swapchain_image->image_view = createImgViewResult.value;
        }

        BRR_LogInfo("Swapchain ImagesResources initialized.");
    }

    void VulkanRenderDevice::Init_SwapchainSynchronization(Swapchain& swapchain)
    {
        for (int i = 0; i < FRAME_LAG; i++)
        {
             auto createImgAvailableSemaphoreResult = m_device.createSemaphore(vk::SemaphoreCreateInfo{});
             if (createImgAvailableSemaphoreResult.result != vk::Result::eSuccess)
             {
                 BRR_LogError("Could not create Image Available Semaphore for swapchain! Result code: {}.", vk::to_string(createImgAvailableSemaphoreResult.result).c_str());
                 exit(1);
             }
             swapchain.image_available_semaphores[i] = createImgAvailableSemaphoreResult.value;
        }

        BRR_LogInfo("Created Swapchain synchronization semaphores and fences");
    }

    void VulkanRenderDevice::Cleanup_Swapchain(Swapchain& swapchain)
    {
        for (int i = 0; i < swapchain.image_resources.size(); ++i)
        {
            Texture2DHandle& swapchain_image_handle = swapchain.image_resources[i];
            Texture2D* swapchain_image = m_texture2d_alloc.GetResource(swapchain_image_handle);
            swapchain_image->image = VK_NULL_HANDLE;
            if (swapchain_image->image_view)
            {
                m_device.destroyImageView(swapchain_image->image_view);
                swapchain_image->image_view = VK_NULL_HANDLE;
                BRR_LogInfo("ImageView of Swapchain Image {} Destroyed.", i);
            }
            m_texture2d_alloc.DestroyResource(swapchain.image_resources[i]);
            swapchain.image_resources[i] = null_handle;
        }

        if (swapchain.swapchain)
        {
            m_device.destroySwapchainKHR(swapchain.swapchain);
            swapchain.swapchain = VK_NULL_HANDLE;
            BRR_LogInfo("Swapchain Destroyed.");
        }
    }

    /************************
     * Descriptor Functions *
     ************************/

    DescriptorLayoutHandle VulkanRenderDevice::CreateDescriptorSetLayout(const DescriptorLayoutBindings& descriptor_layout_bindings)
    {
        return m_descriptor_layout_cache->CreateDescriptorLayout(descriptor_layout_bindings);
    }

    std::vector<DescriptorSetHandle> VulkanRenderDevice::AllocateDescriptorSet(DescriptorLayoutHandle descriptor_layout,
                                                                               uint32_t number_sets)
    {
        vk::DescriptorSetLayout descriptor_set_layout = m_descriptor_layout_cache->GetDescriptorLayout(descriptor_layout);
        std::vector<vk::DescriptorSetLayout> vk_layouts (number_sets, descriptor_set_layout);

        std::vector<vk::DescriptorSet> descriptor_sets;
        bool success = m_descriptor_allocator->Allocate (vk_layouts, descriptor_sets);
        if (!success)
        {
            BRR_LogError("DescriptorSet allocation failed.");
            return {};
        }

        std::vector<DescriptorSetHandle> descriptor_sets_handles (number_sets, null_handle);
        for (uint32_t idx = 0; idx < number_sets; idx++)
        {
            descriptor_sets_handles[idx] = m_descriptor_set_alloc.CreateResource();
            DescriptorSet* descriptor_set = m_descriptor_set_alloc.GetResource(descriptor_sets_handles[idx]);

            descriptor_set->descriptor_set = descriptor_sets[idx];
        }

        return descriptor_sets_handles;
    }

    bool VulkanRenderDevice::UpdateDescriptorSetResources(DescriptorSetHandle descriptor_set_handle,
                                                          const std::vector<DescriptorSetBinding>& shader_bindings)
    {
        DescriptorSet* descriptor_set = m_descriptor_set_alloc.GetResource(descriptor_set_handle);
        if (!descriptor_set)
        {
            return false;
        }

        std::vector<vk::WriteDescriptorSet> descriptor_writes (shader_bindings.size());
        std::vector<vk::DescriptorBufferInfo> desc_buffer_infos;
        desc_buffer_infos.reserve(shader_bindings.size());
        std::vector<vk::DescriptorImageInfo> desc_image_infos;
        desc_image_infos.reserve(shader_bindings.size());
        for (uint32_t binding = 0; binding< descriptor_writes.size(); binding++)
        {
            vk::DescriptorType descriptor_type = VkHelpers::VkDescriptorTypeFromDescriptorType(shader_bindings[binding].descriptor_type);

            vk::WriteDescriptorSet& write = descriptor_writes[binding];
            write
                .setDstBinding(shader_bindings[binding].descriptor_binding)
                .setDstSet(descriptor_set->descriptor_set)
                .setDescriptorType(descriptor_type)
                .setDescriptorCount(1);

            if (shader_bindings[binding].buffer_handle != null_handle)
            {
                Buffer* buffer = m_buffer_alloc.GetResource(shader_bindings[binding].buffer_handle);
                if (!buffer)
                {
                    BRR_LogError("Invalid BufferHandle passed as ShaderBinding. Could not update DescriptorSets succesfully.");
                    break;
                }

                uint32_t buffer_size = shader_bindings[binding].buffer_size;
                if (shader_bindings[binding].buffer_size > buffer->buffer_size)
                {
                    BRR_LogError("Trying to bind buffer with size bigger than the buffer total size. Reverting to buffer size.");
                    buffer_size = buffer->buffer_size;
                }
                
                vk::DescriptorBufferInfo& buffer_info = desc_buffer_infos.emplace_back(buffer->buffer, 0, buffer_size);

                write.setBufferInfo(buffer_info);
            }
            if (shader_bindings[binding].texture_handle != null_handle)
            {
                Texture2D* image = m_texture2d_alloc.GetResource(shader_bindings[binding].texture_handle);
                if (!image)
                {
                    BRR_LogError("Invalid Texture2DHandle passed as ShaderBinding. Could not update DescriptorSets succesfully.");
                    break;
                }
                
                vk::DescriptorImageInfo& image_info = desc_image_infos.emplace_back(m_texture2DSampler, image->image_view, image->image_layout);

                write.setImageInfo(image_info);
            }
        }
        m_device.updateDescriptorSets(descriptor_writes, {});
        return true;
    }

    /********************
     * Buffer Functions *
     ********************/

    BufferHandle VulkanRenderDevice::CreateBuffer(size_t buffer_size, BufferUsage buffer_usage,
                                                  MemoryUsage memory_usage)
    {
        BufferHandle buffer_handle;
        // Create Buffer
        {
            const ResourceHandle resource_handle = m_buffer_alloc.CreateResource();
            buffer_handle.index = resource_handle.index; buffer_handle.validation = resource_handle.validation;
            Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);

            vk::SharingMode sharing_mode = IsDifferentTransferQueue() ? vk::SharingMode::eExclusive : vk::SharingMode::eExclusive;

            const vk::BufferUsageFlags vk_buffer_usage = VkBufferUsageFromDeviceBufferUsage(buffer_usage);

            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info
                .setUsage(vk_buffer_usage)
                .setSharingMode(sharing_mode)
                .setSize(buffer_size);
            if (sharing_mode == vk::SharingMode::eConcurrent)
            {
                std::array<uint32_t, 2> indices{
                    GetQueueFamilyIndices().m_graphicsFamily.value(),
                    GetQueueFamilyIndices().m_transferFamily.value()
                };
                buffer_create_info.setQueueFamilyIndices(indices);
            }

            VmaAllocationCreateFlags alloc_create_flag = 0;
            if ((buffer_usage & BufferUsage::HostAccessSequencial) != 0)
            {
                alloc_create_flag |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }
            if ((buffer_usage & BufferUsage::HostAccessRandom) != 0)
            {
                alloc_create_flag |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            }

            VmaMemoryUsage vma_memory_usage = VmaMemoryUsageFromDeviceMemoryUsage(memory_usage);

            VmaAllocationCreateInfo alloc_info;
            alloc_info.usage = vma_memory_usage;
            alloc_info.flags = alloc_create_flag;
            alloc_info.requiredFlags = 0;
            alloc_info.preferredFlags = 0;
            alloc_info.memoryTypeBits = 0;
            alloc_info.pool = VK_NULL_HANDLE;
            alloc_info.pUserData = nullptr;
            alloc_info.priority = 1.0;

            VkBuffer new_buffer;
            VmaAllocation allocation;
            VmaAllocationInfo allocation_info;
            const vk::Result createBufferResult = vk::Result(vmaCreateBuffer(m_vma_allocator, reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &alloc_info,
                                                                             &new_buffer, &allocation, &allocation_info));

            if (createBufferResult != vk::Result::eSuccess)
            {
                BRR_LogError("Could not create Buffer! Result code: {}.", vk::to_string(createBufferResult).c_str());
                return BufferHandle();
            }
            buffer->buffer = new_buffer;
            buffer->buffer_allocation = allocation;
            buffer->allocation_info = allocation_info;
            buffer->buffer_size = buffer_size;
            buffer->buffer_usage = vk_buffer_usage;

            BRR_LogDebug("Buffer created. Buffer: {:#x}.", size_t(VkBuffer(buffer->buffer)));
        }

        return buffer_handle;
    }

    bool VulkanRenderDevice::DestroyBuffer(BufferHandle buffer_handle)
    {
        Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);
        if (!buffer)
        {
            return false;
        }

        Frame& current_frame = GetCurrentFrame();
        current_frame.buffer_delete_list.emplace_back(buffer->buffer, buffer->buffer_allocation);

        m_buffer_alloc.DestroyResource(buffer_handle);

        BRR_LogDebug("Buffer destroyed. Buffer: {:#x}.", size_t(VkBuffer(buffer->buffer)));
        return true;
    }

    void* VulkanRenderDevice::MapBuffer(BufferHandle buffer_handle)
    {
        Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);

        if (buffer->mapped)
        {
            return buffer->mapped;
        }

        vk::Result result = (vk::Result)vmaMapMemory(m_vma_allocator, buffer->buffer_allocation, &buffer->mapped);
        if (result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not map buffer memory! Result code: {}.", vk::to_string(result).c_str());
            buffer->mapped = nullptr;
        }

        BRR_LogTrace("Mapped buffer. Buffer: {:#x}. Mapped adress: {:#x}.", size_t(VkBuffer(buffer->buffer)), (size_t)buffer->mapped);

        return buffer->mapped;
    }

    void VulkanRenderDevice::UnmapBuffer(BufferHandle buffer_handle)
    {
        Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);

        if (!buffer->mapped)
        {
            return;
        }

        vmaUnmapMemory(m_vma_allocator, buffer->buffer_allocation);
        buffer->mapped = nullptr;

        BRR_LogTrace("Unmapped buffer. Buffer: {:#x}.", size_t(VkBuffer(buffer->buffer)));
    }

    bool VulkanRenderDevice::UploadBufferData(BufferHandle dst_buffer_handle, void* data, size_t size, uint32_t offset)
    {
        Buffer* dst_buffer = m_buffer_alloc.GetResource(dst_buffer_handle);

        UpdateBufferData(dst_buffer->buffer, data, size, 0, offset);

        vk::PipelineStageFlags2 pipeline_stage_flags;
        vk::AccessFlags2 access_flags;
        if (dst_buffer->buffer_usage | vk::BufferUsageFlagBits::eVertexBuffer)
        {
            pipeline_stage_flags = vk::PipelineStageFlagBits2::eVertexAttributeInput;
            access_flags = vk::AccessFlagBits2::eVertexAttributeRead;
        }
        else if (dst_buffer->buffer_usage | vk::BufferUsageFlagBits::eIndexBuffer)
        {
            pipeline_stage_flags = vk::PipelineStageFlagBits2::eIndexInput;
            access_flags = vk::AccessFlagBits2::eIndexRead;
        }
        else if (dst_buffer->buffer_usage | vk::BufferUsageFlagBits::eUniformBuffer)
        {
            pipeline_stage_flags = vk::PipelineStageFlagBits2::eFragmentShader;
            access_flags = vk::AccessFlagBits2::eShaderRead;
        }
        else if (dst_buffer->buffer_usage | vk::BufferUsageFlagBits::eStorageBuffer)
        {
            pipeline_stage_flags = vk::PipelineStageFlagBits2::eFragmentShader;
        }
           

        vk::CommandBuffer transfer_cmd_buffer = GetCurrentTransferCommandBuffer();
        vk::CommandBuffer grapics_cmd_buffer = GetCurrentGraphicsCommandBuffer();
        if (IsDifferentTransferQueue())
        {
            uint32_t src_queue_index = GetQueueFamilyIndices().m_transferFamily.value();
            uint32_t dst_queue_index = GetQueueFamilyIndices().m_graphicsFamily.value();
            { // Yield ownership from transfer -> graphics
                BufferMemoryBarrier(transfer_cmd_buffer, dst_buffer->buffer, dst_buffer->buffer_size, offset,
                                    vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                    vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
                                    src_queue_index, dst_queue_index);
            }
            { // Obtain ownership from transfer -> graphics
                BufferMemoryBarrier(grapics_cmd_buffer, dst_buffer->buffer, dst_buffer->buffer_size, offset,
                                    vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                    pipeline_stage_flags, access_flags,
                                    src_queue_index, dst_queue_index);
            }
        }
        else
        {
            BufferMemoryBarrier(grapics_cmd_buffer, dst_buffer->buffer, dst_buffer->buffer_size, offset,
                                    vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                    pipeline_stage_flags, access_flags);
        }

        return true;
    }

    bool VulkanRenderDevice::CopyBuffer(BufferHandle src_buffer_handle, BufferHandle dst_buffer_handle, size_t size,
                                        uint32_t src_buffer_offset, uint32_t dst_buffer_offset, bool use_transfer_queue)
    {
        Frame& current_frame = GetCurrentFrame();

        vk::CommandBuffer cmd_buffer = use_transfer_queue? current_frame.transfer_cmd_buffer : current_frame.graphics_cmd_buffer;

        Buffer* src_buffer = m_buffer_alloc.GetResource(src_buffer_handle);
        Buffer* dst_buffer = m_buffer_alloc.GetResource(dst_buffer_handle);

        vk::BufferCopy copy_region{};
        copy_region
            .setSrcOffset(src_buffer_offset)
            .setDstOffset(dst_buffer_offset)
            .setSize(size);

        cmd_buffer.copyBuffer(src_buffer->buffer, dst_buffer->buffer, copy_region);

        vk::PipelineStageFlags2 pipeline_stage_flags;
        vk::AccessFlags2 access_flags;
        if (dst_buffer->buffer_usage | vk::BufferUsageFlagBits::eVertexBuffer)
        {
            pipeline_stage_flags = vk::PipelineStageFlagBits2::eVertexAttributeInput;
            access_flags = vk::AccessFlagBits2::eVertexAttributeRead;
        }
        else if (dst_buffer->buffer_usage | vk::BufferUsageFlagBits::eIndexBuffer)
        {
            pipeline_stage_flags = vk::PipelineStageFlagBits2::eIndexInput;
            access_flags = vk::AccessFlagBits2::eIndexRead;
        }
        else if (dst_buffer->buffer_usage | vk::BufferUsageFlagBits::eUniformBuffer)
        {
            pipeline_stage_flags = vk::PipelineStageFlagBits2::eFragmentShader;
            access_flags = vk::AccessFlagBits2::eShaderRead;
        }
        else if (dst_buffer->buffer_usage | vk::BufferUsageFlagBits::eStorageBuffer)
        {
            pipeline_stage_flags = vk::PipelineStageFlagBits2::eFragmentShader;
            access_flags = vk::AccessFlagBits2::eShaderWrite;
        }

        //TODO: Check if src buffer is on CPU or GPU to decide if uses transfer queue or no.
        if (use_transfer_queue)
        {
            uint32_t src_queue_index = GetQueueFamilyIndices().m_transferFamily.value();
            uint32_t dst_queue_index = GetQueueFamilyIndices().m_graphicsFamily.value();
            { // Yield ownership from transfer -> graphics
                BufferMemoryBarrier(current_frame.transfer_cmd_buffer, dst_buffer->buffer, dst_buffer->buffer_size, dst_buffer_offset,
                                    vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                    vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
                                    src_queue_index, dst_queue_index);
            }
            { // Obtain ownership from transfer -> graphics
                BufferMemoryBarrier(current_frame.graphics_cmd_buffer, dst_buffer->buffer, dst_buffer->buffer_size, dst_buffer_offset,
                                    vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                    pipeline_stage_flags, access_flags,
                                    src_queue_index, dst_queue_index);
            }
        }
        else
        {
            BufferMemoryBarrier(current_frame.graphics_cmd_buffer, dst_buffer->buffer, dst_buffer->buffer_size, dst_buffer_offset,
                                    vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                    pipeline_stage_flags, access_flags);
        }

        return true;
    }

    bool VulkanRenderDevice::CopyBuffer_Immediate(BufferHandle src_buffer_handle, BufferHandle dst_buffer_handle,
                                                  size_t size,
                                                  uint32_t src_buffer_offset, uint32_t dst_buffer_offset)
    {
        Buffer* src_buffer = m_buffer_alloc.GetResource(src_buffer_handle);
        Buffer* dst_buffer = m_buffer_alloc.GetResource(dst_buffer_handle);

        const vk::CommandPool transfer_cmd_pool = (IsDifferentTransferQueue()) ? m_transfer_command_pool : m_graphics_command_pool;

        vk::CommandBufferAllocateInfo cmd_buffer_alloc_info{};
        cmd_buffer_alloc_info
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandPool(transfer_cmd_pool)
            .setCommandBufferCount(1);

        auto allocCmdBuffersResult = m_device.allocateCommandBuffers(cmd_buffer_alloc_info);
        if (allocCmdBuffersResult.result != vk::Result::eSuccess)
        {
            BRR_LogError("ERROR: Could not allocate CommandBuffer! Result code: {}.", vk::to_string(allocCmdBuffersResult.result).c_str());
            return false;
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

        cmd_buffer.copyBuffer(src_buffer->buffer, dst_buffer->buffer, copy_region);

        cmd_buffer.end();

        WaitIdle();

        SubmitTransferCommandBuffers(1, &cmd_buffer, 0, nullptr, nullptr, 0, nullptr, VK_NULL_HANDLE);

        m_transfer_queue.waitIdle();

        m_device.freeCommandBuffers(transfer_cmd_pool, cmd_buffer);

        BRR_LogDebug("Immeadite copy buffers. Src buffer: {:#x}. Dst Buffer: {:#x}. Copy size: {}", size_t(VkBuffer(src_buffer->buffer)), size_t(VkBuffer(dst_buffer->buffer)), size);

        return true;
    }

    /***************************
     * Vertex Buffer Functions *
     ***************************/

    VertexBufferHandle VulkanRenderDevice::CreateVertexBuffer(size_t buffer_size, VertexFormatFlags format, void* data)
    {
        VertexBufferHandle vertex_buffer_handle;
        VertexBuffer* vertex_buffer;
        // Create Vertex Buffer
        {
            const ResourceHandle resource_handle = m_vertex_buffer_alloc.CreateResource();
            if (!resource_handle)
            {
                return {};
            }
            vertex_buffer_handle.index = resource_handle.index; vertex_buffer_handle.validation = resource_handle.validation;
            vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
            assert(vertex_buffer && "VertexBuffer not initialized. Something is very wrong.");

            vk::SharingMode sharing_mode = IsDifferentTransferQueue() ? vk::SharingMode::eExclusive : vk::SharingMode::eExclusive;

            const vk::BufferUsageFlags vk_buffer_usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info
                .setUsage(vk_buffer_usage)
                .setSharingMode(sharing_mode)
                .setSize(buffer_size);
            if (sharing_mode == vk::SharingMode::eConcurrent)
            {
                std::array<uint32_t, 2> indices{
                    GetQueueFamilyIndices().m_graphicsFamily.value(),
                    GetQueueFamilyIndices().m_transferFamily.value()
                };
                buffer_create_info.setQueueFamilyIndices(indices);
            }

            VmaMemoryUsage vma_memory_usage = VMA_MEMORY_USAGE_AUTO;

            VmaAllocationCreateInfo alloc_create_info = {};
            alloc_create_info.usage = vma_memory_usage;
            alloc_create_info.flags = 0;
            alloc_create_info.requiredFlags = 0;
            alloc_create_info.preferredFlags = 0;
            alloc_create_info.memoryTypeBits = 0;
            alloc_create_info.pool = VK_NULL_HANDLE;
            alloc_create_info.pUserData = nullptr;
            alloc_create_info.priority = 1.0;

            VkBuffer new_buffer;
            VmaAllocation allocation;
            VmaAllocationInfo allocation_info;
            const vk::Result createBufferResult = vk::Result(vmaCreateBuffer(m_vma_allocator, reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &alloc_create_info,
                                                                             &new_buffer, &allocation, &allocation_info));

            if (createBufferResult != vk::Result::eSuccess)
            {
                m_vertex_buffer_alloc.DestroyResource(vertex_buffer_handle);
                BRR_LogError("Could not create VertexBuffer! Result code: {}.", vk::to_string(createBufferResult).c_str());
                return {};
            }
            vertex_buffer->buffer = new_buffer;
            vertex_buffer->buffer_allocation = allocation;
            vertex_buffer->allocation_info = allocation_info;
            vertex_buffer->buffer_size = buffer_size;
            vertex_buffer->buffer_format = format;

            BRR_LogDebug("Created vertex buffer. Buffer: {:#x}", (size_t)new_buffer);
        }

        if (data != nullptr)
        {
            UpdateVertexBufferData(vertex_buffer_handle, data, buffer_size, 0);
        }

        return vertex_buffer_handle;
    }

    bool VulkanRenderDevice::DestroyVertexBuffer(VertexBufferHandle vertex_buffer_handle)
    {
        VertexBuffer* vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
        if (!vertex_buffer)
        {
            return false;
        }

        Frame& current_frame = GetCurrentFrame();
        current_frame.buffer_delete_list.emplace_back(vertex_buffer->buffer, vertex_buffer->buffer_allocation);

        BRR_LogDebug("Destroyed vertex buffer. Buffer: {:#x}", (size_t)(static_cast<VkBuffer>(vertex_buffer->buffer)));

        return m_vertex_buffer_alloc.DestroyResource(vertex_buffer_handle);
    }

    bool VulkanRenderDevice::UpdateVertexBufferData(VertexBufferHandle vertex_buffer_handle, void* data,
                                                    size_t data_size, uint32_t dst_offset)
    {
        const VertexBuffer* vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
        if (!vertex_buffer)
        {
            return false;
        }

        UpdateBufferData(vertex_buffer->buffer, data, data_size, 0, dst_offset);

        vk::CommandBuffer transfer_cmd_buffer = GetCurrentTransferCommandBuffer();
        vk::CommandBuffer grapics_cmd_buffer = GetCurrentGraphicsCommandBuffer();
        if (IsDifferentTransferQueue())
        {
            uint32_t src_queue_index = GetQueueFamilyIndices().m_transferFamily.value();
            uint32_t dst_queue_index = GetQueueFamilyIndices().m_graphicsFamily.value();

            BufferMemoryBarrier(transfer_cmd_buffer, vertex_buffer->buffer, vertex_buffer->buffer_size, dst_offset,
                                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
                                src_queue_index, dst_queue_index);

            BufferMemoryBarrier(grapics_cmd_buffer, vertex_buffer->buffer, vertex_buffer->buffer_size, dst_offset,
                                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                vk::PipelineStageFlagBits2::eVertexInput, vk::AccessFlagBits2::eVertexAttributeRead,
                                src_queue_index, dst_queue_index);
        }
        else
        {
            BufferMemoryBarrier(grapics_cmd_buffer, vertex_buffer->buffer, vertex_buffer->buffer_size, dst_offset,
                                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                vk::PipelineStageFlagBits2::eVertexInput,
                                vk::AccessFlagBits2::eVertexAttributeRead);
        }

        return true;
    }

    bool VulkanRenderDevice::BindVertexBuffer(VertexBufferHandle vertex_buffer_handle)
    {
        const VertexBuffer* vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
        if (!vertex_buffer)
        {
            return false;
        }
        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();

        uint32_t offset = 0;

        command_buffer.bindVertexBuffers(0, vertex_buffer->buffer, offset);
        return true;
    }

    /**************************
     * Index Buffer Functions *
     **************************/

    IndexBufferHandle VulkanRenderDevice::CreateIndexBuffer(size_t buffer_size, IndexType format, void* data)
    {
        IndexBufferHandle index_buffer_handle;
        IndexBuffer* index_buffer;
        // Create Vertex Buffer
        {
            const ResourceHandle resource_handle = m_index_buffer_alloc.CreateResource();
            if (!resource_handle)
            {
                return {};
            }
            index_buffer_handle.index = resource_handle.index; index_buffer_handle.validation = resource_handle.validation;
            index_buffer = m_index_buffer_alloc.GetResource(index_buffer_handle);
            assert(index_buffer && "IndexBuffer not initialized. Something is very wrong.");

            vk::SharingMode sharing_mode = IsDifferentTransferQueue() ? vk::SharingMode::eExclusive : vk::SharingMode::eExclusive;

            const vk::BufferUsageFlags vk_buffer_usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;

            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info
                .setUsage(vk_buffer_usage)
                .setSharingMode(sharing_mode)
                .setSize(buffer_size);
            if (sharing_mode == vk::SharingMode::eConcurrent)
            {
                std::array<uint32_t, 2> indices{
                    GetQueueFamilyIndices().m_graphicsFamily.value(),
                    GetQueueFamilyIndices().m_transferFamily.value()
                };
                buffer_create_info.setQueueFamilyIndices(indices);
            }

            VmaMemoryUsage vma_memory_usage = VMA_MEMORY_USAGE_AUTO;

            VmaAllocationCreateInfo alloc_create_info = {};
            alloc_create_info.usage = vma_memory_usage;
            alloc_create_info.flags = 0;
            alloc_create_info.requiredFlags = 0;
            alloc_create_info.preferredFlags = 0;
            alloc_create_info.memoryTypeBits = 0;
            alloc_create_info.pool = VK_NULL_HANDLE;
            alloc_create_info.pUserData = nullptr;
            alloc_create_info.priority = 1.0;

            VkBuffer new_buffer;
            VmaAllocation allocation;
            VmaAllocationInfo allocation_info;
            const vk::Result createBufferResult = vk::Result(vmaCreateBuffer(m_vma_allocator, reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &alloc_create_info,
                                                                             &new_buffer, &allocation, &allocation_info));

            if (createBufferResult != vk::Result::eSuccess)
            {
                m_index_buffer_alloc.DestroyResource(index_buffer_handle);
                BRR_LogError("Could not create IndexBuffer! Result code: {}.", vk::to_string(createBufferResult).c_str());
                return {};
            }
            index_buffer->buffer = new_buffer;
            index_buffer->buffer_allocation = allocation;
            index_buffer->allocation_info = allocation_info;
            index_buffer->buffer_size = buffer_size;
            index_buffer->buffer_format = format;

            BRR_LogDebug("Created index buffer. Buffer: {:#x}", (size_t)new_buffer);
        }

        if (data != nullptr)
        {
            UpdateIndexBufferData(index_buffer_handle, data, buffer_size, 0);
        }

        return index_buffer_handle;
    }

    bool VulkanRenderDevice::DestroyIndexBuffer(IndexBufferHandle index_buffer_handle)
    {
        IndexBuffer* index_buffer = m_index_buffer_alloc.GetResource(index_buffer_handle);
        if (!index_buffer)
        {
            return false;
        }

        Frame& current_frame = GetCurrentFrame();
        current_frame.buffer_delete_list.emplace_back(index_buffer->buffer, index_buffer->buffer_allocation);

        BRR_LogDebug("Destroyed index buffer. Buffer: {:#x}", (size_t)(static_cast<VkBuffer>(index_buffer->buffer)));

        return m_index_buffer_alloc.DestroyResource(index_buffer_handle);
    }

    bool VulkanRenderDevice::UpdateIndexBufferData(IndexBufferHandle index_buffer_handle, void* data, size_t data_size,
                                                   uint32_t dst_offset)
    {
        const IndexBuffer* index_buffer = m_index_buffer_alloc.GetResource(index_buffer_handle);
        if (!index_buffer)
        {
            return false;
        }

        UpdateBufferData(index_buffer->buffer, data, data_size, 0, dst_offset);

        vk::CommandBuffer transfer_cmd_buffer = GetCurrentTransferCommandBuffer();
        vk::CommandBuffer grapics_cmd_buffer = GetCurrentGraphicsCommandBuffer();
        if (IsDifferentTransferQueue())
        {
            uint32_t src_queue_index = GetQueueFamilyIndices().m_transferFamily.value();
            uint32_t dst_queue_index = GetQueueFamilyIndices().m_graphicsFamily.value();

            BufferMemoryBarrier(transfer_cmd_buffer, index_buffer->buffer, index_buffer->buffer_size, dst_offset,
                                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
                                src_queue_index, dst_queue_index);

            BufferMemoryBarrier(grapics_cmd_buffer, index_buffer->buffer, index_buffer->buffer_size, dst_offset,
                                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                vk::PipelineStageFlagBits2::eVertexInput, vk::AccessFlagBits2::eVertexAttributeRead,
                                src_queue_index, dst_queue_index);
        }
        else
        {
            BufferMemoryBarrier(grapics_cmd_buffer, index_buffer->buffer, index_buffer->buffer_size, dst_offset,
                                vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
                                vk::PipelineStageFlagBits2::eVertexInput,
                                vk::AccessFlagBits2::eVertexAttributeRead);
        }

        return true;
    }

    bool VulkanRenderDevice::BindIndexBuffer(IndexBufferHandle index_buffer_handle)
    {
        const IndexBuffer* index_buffer = m_index_buffer_alloc.GetResource(index_buffer_handle);
        if (!index_buffer)
        {
            return false;
        }

        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();

        vk::IndexType index_type;
        switch (index_buffer->buffer_format)
        {
        case IndexType::UINT8: 
            index_type = vk::IndexType::eUint8EXT;
            break;
        case IndexType::UINT16: 
            index_type = vk::IndexType::eUint16;
            break;
        case IndexType::UINT32: 
            index_type = vk::IndexType::eUint32;
            break;
        default: 
            index_type = vk::IndexType::eNoneKHR;
            break;
        }

        command_buffer.bindIndexBuffer(index_buffer->buffer, 0, index_type);

        return true;
    }

    /***********************
     * Texture2D Functions *
     ***********************/

    Texture2DHandle VulkanRenderDevice::Create_Texture2D(uint32_t width, uint32_t height, ImageUsage image_usage, DataFormat image_format)
    {
        Texture2DHandle texture2d_handle;
        Texture2D* texture2d;
        const ResourceHandle resource_handle = m_texture2d_alloc.CreateResource();
        if (!resource_handle)
        {
            return {};
        }
        texture2d_handle.index = resource_handle.index; texture2d_handle.validation = resource_handle.validation;
        texture2d = m_texture2d_alloc.GetResource(texture2d_handle);
        assert(texture2d && "Texture2D not initialized. Something is very wrong.");
        
        vk::ImageUsageFlags vk_image_usage = VkImageUsageFromDeviceImageUsage(image_usage);
        vk::Format vk_format = VkHelpers::VkFormatFromDeviceDataFormat(image_format);

        vk::ImageCreateInfo img_create_info;
        img_create_info
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setUsage(vk_image_usage)
            .setExtent(vk::Extent3D(width, height, 1))
            .setFormat(vk_format)
            .setImageType(vk::ImageType::e2D)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setMipLevels(1)
            .setArrayLayers(1);

        VmaAllocationCreateInfo alloc_create_info;
        alloc_create_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_create_info.flags = 0;
        alloc_create_info.requiredFlags = 0;
        alloc_create_info.preferredFlags = 0;
        alloc_create_info.memoryTypeBits = 0;
        alloc_create_info.pool = VK_NULL_HANDLE;
        alloc_create_info.pUserData = nullptr;
        alloc_create_info.priority = 1.0;

        VkImage new_image;
        VmaAllocation allocation;
        VmaAllocationInfo allocation_info;
        const vk::Result createImageResult = vk::Result(vmaCreateImage(m_vma_allocator,
                                                                       reinterpret_cast<VkImageCreateInfo*>
                                                                       (&img_create_info),
                                                                       &alloc_create_info, &new_image,
                                                                       &allocation, &allocation_info));
        if (createImageResult != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create Image! Result code: {}.", vk::to_string(createImageResult).c_str());
            return {};
        }

        BRR_LogInfo("Image created.");

        // Create ImageView

        vk::ImageAspectFlags img_aspect_flags = VkHelpers::IsDepthStencilDataFormat(image_format)
                                                    ? vk::ImageAspectFlagBits::eDepth
                                                    : vk::ImageAspectFlagBits::eColor;

        vk::ImageSubresourceRange subresource_range;
        subresource_range
            .setAspectMask(img_aspect_flags)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1);

        vk::ImageViewCreateInfo view_create_info;
        view_create_info
            .setImage(new_image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk_format)
            .setSubresourceRange(subresource_range);

        auto view_result = m_device.createImageView(view_create_info);
        if (view_result.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create ImageView! Result code: {}.", vk::to_string(view_result.result).c_str());
            vmaDestroyImage(m_vma_allocator, new_image, allocation);
            return {};
        }

        texture2d->image = new_image;
        texture2d->image_view = view_result.value;
        texture2d->image_allocation = allocation;
        texture2d->allocation_info = allocation_info;
        texture2d->image_extent = vk::Extent2D{width, height};
        texture2d->image_format = vk_format;

        if ((image_usage & ImageUsage::SampledImage) != 0
         || (image_usage & ImageUsage::InputAttachmentImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        }
        //else if ((image_usage & ImageUsage::StorageImage) != 0)
        //{
        //    // TODO
        //    //texture2d->image_layout = vk::ImageLayout::
        //}
        else if ((image_usage & ImageUsage::ColorAttachmentImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eColorAttachmentOptimal;
        }
        else if ((image_usage & ImageUsage::DepthStencilAttachmentImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }
        else if ((image_usage & ImageUsage::TransferSrcImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eTransferSrcOptimal;
        }
        else if ((image_usage & ImageUsage::TransferDstImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eTransferDstOptimal;
        }

        return texture2d_handle;
    }

    bool VulkanRenderDevice::DestroyTexture2D(Texture2DHandle texture2d_handle)
    {
        const Texture2D* texture = m_texture2d_alloc.GetResource(texture2d_handle);
        if (!texture)
        {
            return false;
        }

        Frame& current_frame = GetCurrentFrame();
        current_frame.texture_delete_list.emplace_back(texture->image, texture->image_view, texture->image_allocation);

        BRR_LogDebug("Destroyed Texture2D. Image: {:#x}", (size_t)(static_cast<VkImage>(texture->image)));

        return m_texture2d_alloc.DestroyResource(texture2d_handle);
    }

    bool VulkanRenderDevice::UpdateTexture2DData(Texture2DHandle texture2d_handle, const void* data, size_t buffer_size,
                                                 const glm::ivec2& image_offset, const glm::uvec2& image_extent)
    {
        const Texture2D* texture = m_texture2d_alloc.GetResource(texture2d_handle);
        if (!texture)
        {
            return false;
        }

        vk::CommandBuffer cmd_buffer = GetCurrentGraphicsCommandBuffer();
        vk::CommandBuffer transfer_cmd_buffer = GetCurrentTransferCommandBuffer();

        {
            TransitionImageLayout(transfer_cmd_buffer, texture->image,
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                  vk::AccessFlagBits2::eNone, vk::PipelineStageFlagBits2::eNone,
                                  vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer,
                                  vk::ImageAspectFlagBits::eColor);
        }

        for (uint32_t y = 0; y < image_extent.y; y += IMAGE_TRANSFER_BLOCK_SIZE)
        {
            for (uint32_t x = 0; x < image_extent.x; x += IMAGE_TRANSFER_BLOCK_SIZE)
            {
                glm::uvec2 block_size = {
                    std::min(IMAGE_TRANSFER_BLOCK_SIZE, image_extent.x - x),
                    std::min(IMAGE_TRANSFER_BLOCK_SIZE, image_extent.y - y)
                };
                uint32_t block_size_bytes = block_size.x * block_size.y * 4;
                StagingBufferHandle staging_buffer{};
                m_staging_allocator.AllocateStagingBuffer(m_current_frame, block_size_bytes, &staging_buffer, false);

                m_staging_allocator.WriteBlockImageToStaging (staging_buffer, static_cast<const uint8_t*>(data), {x, y}, block_size, image_extent, 4); //(staging_buffer, 0, buffer.data(), block_size_bytes);

                m_staging_allocator.CopyFromStagingToImage(staging_buffer, GetCurrentTransferCommandBuffer(), texture->image,
                                                           {block_size.x, block_size.y, 1}, 0, {static_cast<int32_t>(x), static_cast<int32_t>(y), 0});
            }
        }

        if (IsDifferentTransferQueue())
        {
            TransitionImageLayout(transfer_cmd_buffer, texture->image,
                                  vk::ImageLayout::eTransferDstOptimal, texture->image_layout,
                                  vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eNone, vk::PipelineStageFlagBits2::eNone,
                                  vk::ImageAspectFlagBits::eColor, GetQueueFamilyIndices().m_transferFamily.value(),
                                  GetQueueFamilyIndices().m_graphicsFamily.value());

            TransitionImageLayout(cmd_buffer, texture->image,
                                  vk::ImageLayout::eTransferDstOptimal, texture->image_layout,
                                  vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::ImageAspectFlagBits::eColor, GetQueueFamilyIndices().m_transferFamily.value(),
                                  GetQueueFamilyIndices().m_graphicsFamily.value());
        }
        else
        {
            TransitionImageLayout(cmd_buffer, texture->image,
                                  vk::ImageLayout::eTransferDstOptimal, texture->image_layout,
                                  vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::ImageAspectFlagBits::eColor);
        }
        return true;
    }

    void VulkanRenderDevice::Texture2D_Blit(Texture2DHandle src_texture2d_handle, Texture2DHandle dst_texture2d_handle)
    {
        if (!m_texture2d_alloc.OwnsResource(src_texture2d_handle))
        {
            BRR_LogError("Can't blit Texture2D. Src Texture2D doesn't exist.");
            return;
        }

        if (!m_texture2d_alloc.OwnsResource(src_texture2d_handle))
        {
            BRR_LogError("Can't blit Texture2D. Dst Texture2D doesn't exist.");
            return;
        }
        Texture2D* src_image = m_texture2d_alloc.GetResource(src_texture2d_handle);
        Texture2D* dst_image = m_texture2d_alloc.GetResource(dst_texture2d_handle);

        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();

        // Src Image transition from Undefined to TransferSrc
        {
            TransitionImageLayout(command_buffer, src_image->image,
                                  src_image->image_layout,
                                  vk::ImageLayout::eTransferSrcOptimal,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::AccessFlagBits2::eMemoryRead,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                  vk::ImageAspectFlagBits::eColor);
        }

        // Swapchain Image transition from Undefined to TransferDst
        {
            TransitionImageLayout(command_buffer, dst_image->image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eTransferDstOptimal,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                  vk::ImageAspectFlagBits::eColor);
        }

        vk::ImageBlit2 image_blit {};
        image_blit.srcOffsets[1]
            .setX(src_image->image_extent.width)
            .setY(src_image->image_extent.height)
            .setZ(1);

        image_blit.dstOffsets[1]
            .setX(dst_image->image_extent.width)
            .setY(dst_image->image_extent.height)
            .setZ(1);

        image_blit.srcSubresource
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setMipLevel(0);

        image_blit.dstSubresource
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setMipLevel(0);

        vk::BlitImageInfo2 blit_info {};
        blit_info
            .setDstImage(dst_image->image)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcImage(src_image->image)
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setFilter(vk::Filter::eLinear)
            .setRegionCount(1)
            .setRegions(image_blit);

        command_buffer.blitImage2(blit_info);

        // Src Image transition from TransferSrc to image`s default layout
        {
            TransitionImageLayout(command_buffer, src_image->image,
                                  vk::ImageLayout::eTransferSrcOptimal,
                                  src_image->image_layout,
                                  vk::AccessFlagBits2::eMemoryRead,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::ImageAspectFlagBits::eColor);
        }

        // Image transition from TransferDst to PresentSrc
        {
            TransitionImageLayout(command_buffer, dst_image->image,
                                  vk::ImageLayout::eTransferDstOptimal,
                                  dst_image->image_layout,
                                  vk::AccessFlagBits2::eMemoryWrite,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                                  vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::ImageAspectFlagBits::eColor);
        }
    }

    /*******************************
     * Graphics Pipeline Functions *
     *******************************/

    //
    ResourceHandle VulkanRenderDevice::Create_GraphicsPipeline(const Shader& shader, 
                                                               const std::vector<DataFormat>& color_attachment_formats,
                                                               DataFormat depth_attachment_format)
    {
        GraphicsPipeline* graphics_pipeline;
        const ResourceHandle pipeline_handle = m_graphics_pipeline_alloc.CreateResource();
        if (!pipeline_handle)
        {
            return {};
        }
        graphics_pipeline = m_graphics_pipeline_alloc.GetResource(pipeline_handle);

        vk::PipelineVertexInputStateCreateInfo vertex_input_info = shader.GetPipelineVertexInputState();

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
        input_assembly_info
            .setTopology(vk::PrimitiveTopology::eTriangleList)
            .setPrimitiveRestartEnable(false);

        vk::PipelineViewportStateCreateInfo viewport_state_info{};
        viewport_state_info
            .setViewportCount(1)
            .setScissorCount(1);

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info {};
        depth_stencil_state_create_info
            .setDepthTestEnable(VK_TRUE)
            .setDepthWriteEnable(VK_TRUE)
            .setDepthCompareOp(vk::CompareOp::eLess)
            .setDepthBoundsTestEnable(VK_FALSE)
            .setMinDepthBounds(0.0)
            .setMaxDepthBounds(1.0)
            .setStencilTestEnable(VK_FALSE);

        vk::PipelineRasterizationStateCreateInfo rasterization_state_info{};
        rasterization_state_info
            .setDepthClampEnable(false)
            .setRasterizerDiscardEnable(false)
            .setPolygonMode(vk::PolygonMode::eFill)
            .setLineWidth(1.f)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eCounterClockwise)
            .setDepthBiasEnable(false)
            .setDepthBiasConstantFactor(0.f)
            .setDepthBiasClamp(0.f)
            .setDepthBiasSlopeFactor(0.f);

        vk::PipelineMultisampleStateCreateInfo multisampling_info{};
        multisampling_info
            .setSampleShadingEnable(false)
            .setRasterizationSamples(vk::SampleCountFlagBits::e1)
            .setMinSampleShading(1.f)
            .setPSampleMask(nullptr)
            .setAlphaToCoverageEnable(false)
            .setAlphaToOneEnable(false);

        vk::PipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment
            .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
            .setBlendEnable(false)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
            .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd);

        vk::PipelineColorBlendStateCreateInfo color_blending_info{};
        color_blending_info
            .setLogicOpEnable(false)
            .setLogicOp(vk::LogicOp::eCopy)
            .setAttachments(color_blend_attachment);

#if 1
        std::vector<vk::DynamicState> dynamic_states{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };

        vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
        dynamic_state_info
            .setDynamicStates(dynamic_states);
#endif

        const std::vector<DescriptorLayout>& desc_set_layouts = shader.GetDescriptorSetLayouts();
        std::vector<vk::DescriptorSetLayout> vk_layouts;
        vk_layouts.reserve(desc_set_layouts.size());
        for (const auto& layout : desc_set_layouts)
        {
            vk_layouts.emplace_back(m_descriptor_layout_cache->GetDescriptorLayout(layout.m_layout_handle));
        }

        vk::PipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info
            .setSetLayouts(vk_layouts);

        auto createPipelineLayoutResult = m_device.createPipelineLayout(pipeline_layout_info);
        if (createPipelineLayoutResult.result != vk::Result::eSuccess)
        {
            BRR_LogError("Not able to create PipelineLayout. Result code: {}.", vk::to_string(createPipelineLayoutResult.result).c_str());
            m_graphics_pipeline_alloc.DestroyResource(pipeline_handle);
            return {};
        }
        graphics_pipeline->pipeline_layout = createPipelineLayoutResult.value;

        std::vector<vk::Format> vk_color_attachment_formats;
        vk_color_attachment_formats.reserve(color_attachment_formats.size());
        for (const auto& format : color_attachment_formats)
        {
            vk::Format vk_format = VkHelpers::VkFormatFromDeviceDataFormat(format);
            vk_color_attachment_formats.emplace_back(vk_format);
        }
        vk::Format vk_depth_attachment_format = VkHelpers::VkFormatFromDeviceDataFormat(depth_attachment_format);

        vk::PipelineRenderingCreateInfo rendering_create_info {};
        rendering_create_info
            .setColorAttachmentFormats(vk_color_attachment_formats)
            .setDepthAttachmentFormat(vk_depth_attachment_format);

        vk::GraphicsPipelineCreateInfo graphics_pipeline_info{};
        graphics_pipeline_info
            .setStages(shader.GetPipelineStagesInfo())
            .setPVertexInputState(&vertex_input_info)
            .setPInputAssemblyState(&input_assembly_info)
            .setPDynamicState(&dynamic_state_info)
            .setPViewportState(&viewport_state_info)
            .setPRasterizationState(&rasterization_state_info)
            .setPMultisampleState(&multisampling_info)
            .setPColorBlendState(&color_blending_info)
            .setPDepthStencilState(&depth_stencil_state_create_info);
        graphics_pipeline_info
            .setPNext(&rendering_create_info)
            .setLayout(graphics_pipeline->pipeline_layout)
            .setSubpass(0)
            .setBasePipelineHandle(VK_NULL_HANDLE)
            .setBasePipelineIndex(-1);

        auto createGraphicsPipelineResult = m_device.createGraphicsPipeline(VK_NULL_HANDLE, graphics_pipeline_info);
        if (createGraphicsPipelineResult.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create GraphicsPipeline! Result code: {}.", vk::to_string(createGraphicsPipelineResult.result).c_str());
            m_graphics_pipeline_alloc.DestroyResource(pipeline_handle);
            return {};
        }

        graphics_pipeline->pipeline = createGraphicsPipelineResult.value;

        BRR_LogInfo("Graphics DevicePipeline created.");

        return pipeline_handle;
    }

    bool VulkanRenderDevice::DestroyGraphicsPipeline(ResourceHandle graphics_pipeline_handle)
    {
        const GraphicsPipeline* graphics_pipeline = m_graphics_pipeline_alloc.GetResource(graphics_pipeline_handle);
        if (!graphics_pipeline)
        {
            return false;
        }

        m_device.destroyPipeline(graphics_pipeline->pipeline);
        m_device.destroyPipelineLayout(graphics_pipeline->pipeline_layout);

        m_graphics_pipeline_alloc.DestroyResource(graphics_pipeline_handle);

        return true;
    }

    void VulkanRenderDevice::Bind_GraphicsPipeline(ResourceHandle graphics_pipeline_handle)
    {
        const GraphicsPipeline* graphics_pipeline = m_graphics_pipeline_alloc.GetResource(graphics_pipeline_handle);
        if (!graphics_pipeline)
        {
            return;
        }

        Frame& current_frame = GetCurrentFrame();

        current_frame.graphics_cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline->pipeline);
    }

    void VulkanRenderDevice::Bind_DescriptorSet(ResourceHandle graphics_pipeline_handle, DescriptorSetHandle descriptor_set_handle, uint32_t set_index)
    {
        const GraphicsPipeline* graphics_pipeline = m_graphics_pipeline_alloc.GetResource(graphics_pipeline_handle);
        if (!graphics_pipeline)
        {
            BRR_LogError ("Trying to bind DescriptorSet with invalid graphics pipeline.");
            return;
        }

        const DescriptorSet* descriptor_set = m_descriptor_set_alloc.GetResource(descriptor_set_handle);
        if (!descriptor_set)
        {
            BRR_LogError ("Trying to bind DescriptorSet with invalid DescriptorSetHandle.");
            return;
        }

        Frame& current_frame = GetCurrentFrame();

        current_frame.graphics_cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                             graphics_pipeline->pipeline_layout, set_index,
                                                             descriptor_set->descriptor_set, {});
    }

    /******************
     * Draw Functions *
     ******************/

    void VulkanRenderDevice::Draw(uint32_t num_vertex, uint32_t num_instances, uint32_t first_vertex,
                                  uint32_t first_instance)
    {
        Frame& current_frame = GetCurrentFrame();

        current_frame.graphics_cmd_buffer.draw(num_vertex, num_instances, first_vertex, first_instance);
    }

    void VulkanRenderDevice::DrawIndexed(uint32_t num_indices, uint32_t num_instances, uint32_t first_index,
                                         uint32_t vertex_offset, uint32_t first_instance)
    {
        Frame& current_frame = GetCurrentFrame();

        current_frame.graphics_cmd_buffer.drawIndexed(num_indices, num_instances, first_index, vertex_offset, first_instance);
    }

    /******************************
     * Vulkan Utilities Functions *
     ******************************/

    void VulkanRenderDevice::UpdateBufferData(vk::Buffer dst_buffer, void* data, size_t size,
                                              uint32_t src_offset, uint32_t dst_offset)
    {
        vk::CommandBuffer transfer_cmd_buffer = GetCurrentTransferCommandBuffer();
        // Make transfer
        uint32_t written_bytes = 0;
        while (written_bytes != size)
        {
            render::StagingBufferHandle staging_buffer{};
            const uint32_t allocated = m_staging_allocator.AllocateStagingBuffer(m_current_frame, size - written_bytes, &staging_buffer);

            m_staging_allocator.WriteLinearBufferToStaging(staging_buffer, 0, static_cast<char*>(data) + written_bytes, allocated);

            m_staging_allocator.CopyFromStagingToBuffer(staging_buffer, transfer_cmd_buffer, dst_buffer, allocated, src_offset, dst_offset + written_bytes);

            written_bytes += allocated;
        }
    }

    bool VulkanRenderDevice::TransitionImageLayout(vk::CommandBuffer cmd_buffer, vk::Image image,
                                                   vk::ImageLayout current_layout, vk::ImageLayout new_layout,
                                                   vk::AccessFlags2 src_access_mask,
                                                   vk::PipelineStageFlags2 src_stage_mask,
                                                   vk::AccessFlags2 dst_access_mask,
                                                   vk::PipelineStageFlags2 dst_stage_mask,
                                                   vk::ImageAspectFlags image_aspect,
                                                   uint32_t src_queue_index, uint32_t dst_queue_index)
    {
        vk::ImageSubresourceRange img_subresource_range;
        img_subresource_range
            .setAspectMask(image_aspect)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1);

        vk::ImageMemoryBarrier2 img_mem_barrier;
        img_mem_barrier
            .setOldLayout(current_layout)
            .setNewLayout(new_layout)
            .setSrcAccessMask(src_access_mask)
            .setDstAccessMask(dst_access_mask)
            .setSrcStageMask(src_stage_mask)
            .setDstStageMask(dst_stage_mask)
            .setSrcQueueFamilyIndex(src_queue_index)
            .setDstQueueFamilyIndex(dst_queue_index)
            .setImage(image)
            .setSubresourceRange(img_subresource_range);

        vk::DependencyInfo dependency_info;
        dependency_info
            .setImageMemoryBarriers(img_mem_barrier);

        cmd_buffer.pipelineBarrier2(dependency_info);

        return true;
    }

    void VulkanRenderDevice::BufferMemoryBarrier(vk::CommandBuffer cmd_buffer, vk::Buffer buffer,
                                                 size_t buffer_size, uint32_t buffer_offset,
                                                 vk::PipelineStageFlags2 src_stage_flags,
                                                 vk::AccessFlags2 src_access_flags,
                                                 vk::PipelineStageFlags2 dst_stage_flags,
                                                 vk::AccessFlags2 dst_access_flags,
                                                 uint32_t src_queue_family, uint32_t dst_queue_family)
    {
        vk::BufferMemoryBarrier2 buffer_memory_barrier;
        buffer_memory_barrier
            .setSrcStageMask(src_stage_flags)
            .setSrcAccessMask(src_access_flags)
            .setDstStageMask(dst_stage_flags)
            .setDstAccessMask(dst_access_flags)
            .setSrcQueueFamilyIndex(src_queue_family)
            .setDstQueueFamilyIndex(dst_queue_family)
            .setBuffer(buffer)
            .setSize(buffer_size)
            .setOffset(buffer_offset);

        vk::DependencyInfo dependency_info;
        dependency_info
            .setBufferMemoryBarriers(buffer_memory_barrier);

        cmd_buffer.pipelineBarrier2(dependency_info);
    }

    vk::Result VulkanRenderDevice::BeginCommandBuffer(vk::CommandBuffer cmd_buffer)
    {
        const vk::Result graph_reset_result = cmd_buffer.reset();
        if (graph_reset_result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not reset command buffer of frame {}. Result code: {}", m_current_frame, vk::to_string(graph_reset_result).c_str());
            return graph_reset_result;
        }

        const vk::CommandBufferBeginInfo cmd_buffer_begin_info{};
        return cmd_buffer.begin(cmd_buffer_begin_info);
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

        return m_graphics_queue.submit(submit_info, submit_fence);
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

        return m_transfer_queue.submit(submit_info, submit_fence);
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

    /****************************
     * Initialization Functions *
     ****************************/

    void VulkanRenderDevice::Init_VkInstance(vis::Window* window)
    {
        // Dynamic load of the library
        {
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = VulkanDynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            if (vkGetInstanceProcAddr == nullptr)
            {
                BRR_LogError("Could not load 'vkGetInstanceProcAddr' function address. Can't create vkInstance.");
                exit(1);
            }
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
            BRR_LogDebug("Loaded 'vkGetInstanceProcAddr' function address. Proceeding to create vkInstance.");
        }

        std::vector<char const*> instance_layers;
        // Check for validation layers support
#ifdef NDEBUG
        constexpr bool use_validation_layers = false;
#else
        constexpr bool use_validation_layers = true;
        instance_layers.emplace_back("VK_LAYER_KHRONOS_validation");
        BRR_LogDebug("Using validation layers.");
#endif

        std::vector<char const*> enabled_layers;
        if (use_validation_layers)
        {
            auto enumInstLayerPropsResult = vk::enumerateInstanceLayerProperties();
            std::vector<vk::LayerProperties> layers = enumInstLayerPropsResult.value;
            std::vector<char const*> acceptedLayers;
            if (VkHelpers::CheckLayers(instance_layers, layers, acceptedLayers))
            {
                enabled_layers = acceptedLayers;
            }
            else
            {
                BRR_LogError("Could not find any of the required validation layers.");
                /*exit(1);*/
            }

            {
                LogStreamBuffer log_msg = BRR_DebugStrBuff();
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
#ifndef NDEBUG
            extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

            // TODO: Check if the required extensions are supported by Vulkan
            uint32_t extension_count = 0;
            auto enumInstExtPropsResult = vk::enumerateInstanceExtensionProperties();
            std::vector<vk::ExtensionProperties> extension_properties = enumInstExtPropsResult.value;

            {
              LogStreamBuffer log_msg = BRR_DebugStrBuff();
                  log_msg << "Available Instance Extensions:";
                for (vk::ExtensionProperties& extension : extension_properties)
                {
                    log_msg << "\n\tExtension name: " << extension.extensionName;
                }
            }

          {
            LogStreamBuffer aLogMsg = BRR_InfoStrBuff();
                aLogMsg << "Required Instance Extensions:";
                for (const char* extension : extensions)
                {
                  aLogMsg << "\n\tExtension name: " << extension;
                }
                aLogMsg.Flush();
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
            .setPEnabledLayerNames(enabled_layers);

         auto createInstanceResult = vk::createInstance(inst_create_info);
         if (createInstanceResult.result != vk::Result::eSuccess)
         {
             BRR_LogError("Could not create Vulkan Instance! Result code: {}.", vk::to_string(createInstanceResult.result).c_str());
             exit(1);
         }
         m_vulkan_instance = createInstanceResult.value;

        {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vulkan_instance);
            BRR_LogInfo("Loaded instance specific vulkan functions addresses.");
        }

         BRR_LogDebug("Instance Created");
    }

    void VulkanRenderDevice::Init_PhysDevice(vk::SurfaceKHR surface)
    {
        auto enumPhysDevicesResult = m_vulkan_instance.enumeratePhysicalDevices();
        std::vector<vk::PhysicalDevice> devices = enumPhysDevicesResult.value;

        if (devices.size() == 0)
        {
            BRR_LogError("Failed to find a physical device with Vulkan support. Exitting program.");
            exit(1);
        }

        {
            LogStreamBuffer log_msg = BRR_DebugStrBuff();
            log_msg << "Available devices:";
            for (vk::PhysicalDevice& device : devices)
            {
                log_msg << "\n\tDevice: " << device.getProperties().deviceName;
            }
        }

        m_phys_device = VkHelpers::Select_PhysDevice(devices, surface);
        std::string device_name = m_phys_device.getProperties().deviceName;

        auto device_extensions = m_phys_device.enumerateDeviceExtensionProperties();
        if (device_extensions.result == vk::Result::eSuccess)
        {
            LogStreamBuffer log_msg = BRR_DebugStrBuff();
            log_msg << "Available Device Extensions (" << device_name << "):";
            for (vk::ExtensionProperties& extension : device_extensions.value)
            {
                log_msg << "\n\tExtension name: " << extension.extensionName;
            }
        }

        m_device_properties = m_phys_device.getProperties2();

        BRR_LogInfo("Selected physical device: {}", m_phys_device.getProperties().deviceName);
    }

    void VulkanRenderDevice::Init_Queues_Indices(vk::SurfaceKHR surface)
    {
        // Check for queue families
        m_queue_family_indices = VkHelpers::Find_QueueFamilies(m_phys_device, surface);

        if (!m_queue_family_indices.m_graphicsFamily.has_value())
        {
            BRR_LogError("Failed to find graphics family queue. Exitting program.");
            exit(1);
        }

        if (!m_queue_family_indices.m_presentFamily.has_value())
        {
            BRR_LogError("Failed to find presentation family queue. Exitting program.");
            exit(1);
        }
    }

    void VulkanRenderDevice::Init_Device()
    {
        if (!m_queue_family_indices.m_graphicsFamily.has_value())
        {
            BRR_LogError("Cannot create device without initializing at least the graphics queue.");
            exit(1);
        }

        const uint32_t graphics_family_idx = m_queue_family_indices.m_graphicsFamily.value();
        const uint32_t presentation_family_idx = m_queue_family_indices.m_presentFamily.value();
        const uint32_t transfer_family_idx = m_queue_family_indices.m_transferFamily.has_value() ? m_queue_family_indices.m_transferFamily.value() : graphics_family_idx;

        BRR_LogDebug("Selected Queue Families:\n"
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

        m_different_present_queue = graphics_family_idx != presentation_family_idx;
        if (m_different_present_queue)
        {
            queues.push_back(vk::DeviceQueueCreateInfo{}
                .setQueueFamilyIndex(presentation_family_idx)
                .setQueuePriorities(priorities));
        }

        m_different_transfer_queue = graphics_family_idx != transfer_family_idx;
        if (m_different_transfer_queue)
        {
            queues.push_back(vk::DeviceQueueCreateInfo{}
                .setQueueFamilyIndex(transfer_family_idx)
                .setQueuePriorities(priorities));
        }

        vk::PhysicalDeviceFeatures device_features{};
        device_features.setSamplerAnisotropy(VK_TRUE);

        std::vector<const char*> device_extensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        vk::PhysicalDeviceSynchronization2Features synchronization2_features {true};

        vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features {true, &synchronization2_features};

        vk::DeviceCreateInfo device_create_info = vk::DeviceCreateInfo{};
        device_create_info
            .setPNext(&dynamic_rendering_features)
            .setQueueCreateInfos(queues)
            .setPEnabledFeatures(&device_features)
            .setEnabledLayerCount(0)
            .setPEnabledExtensionNames(device_extensions);

        auto createDeviceResult = m_phys_device.createDevice(device_create_info);
        if (createDeviceResult.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create Vulkan Device! Result code: {}.", vk::to_string(createDeviceResult.result).c_str());
            exit(1);
        }
        m_device = createDeviceResult.value;
        {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
            BRR_LogDebug("Loaded Device specific Vulkan functions addresses.");
        }

        m_graphics_queue = m_device.getQueue(graphics_family_idx, 0);

        m_presentation_queue = (m_different_present_queue) ? m_device.getQueue(presentation_family_idx, 0) : m_graphics_queue;

        m_transfer_queue = (m_different_transfer_queue) ? m_device.getQueue(transfer_family_idx, 0) : m_graphics_queue;

        BRR_LogDebug("VkDevice Created");
    }

    void VulkanRenderDevice::Init_Allocator()
    {
        VmaVulkanFunctions vulkan_functions;
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
        vma_alloc_create_info.instance = m_vulkan_instance;
        vma_alloc_create_info.physicalDevice = m_phys_device;
        vma_alloc_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
        vma_alloc_create_info.pVulkanFunctions = &vulkan_functions;

        vmaCreateAllocator(&vma_alloc_create_info, &m_vma_allocator);

        BRR_LogDebug("Initialized VMA allocator. Allocator: {:#x}.", (size_t)m_vma_allocator);
    }

    void VulkanRenderDevice::Init_CommandPool()
    {
        vk::CommandPoolCreateInfo command_pool_info{};
        command_pool_info
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(m_queue_family_indices.m_graphicsFamily.value());

         auto createCmdPoolResult = m_device.createCommandPool(command_pool_info);
         if (createCmdPoolResult.result != vk::Result::eSuccess)
         {
             BRR_LogError("Could not create CommandPool! Result code: {}.", vk::to_string(createCmdPoolResult.result).c_str());
             exit(1);
         }
         m_graphics_command_pool = createCmdPoolResult.value;

         BRR_LogInfo("CommandPool created.");

        if (m_different_present_queue)
        {
            vk::CommandPoolCreateInfo present_pool_info{};
            present_pool_info
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(m_queue_family_indices.m_presentFamily.value());

             auto createPresentCmdPoolResult = m_device.createCommandPool(present_pool_info);
             if (createPresentCmdPoolResult.result != vk::Result::eSuccess)
             {
                 BRR_LogError("Could not create present CommandPool! Result code: {}.", vk::to_string(createPresentCmdPoolResult.result).c_str());
                 exit(1);
             }
             m_present_command_pool = createPresentCmdPoolResult.value;

             BRR_LogInfo("Separate Present CommandPool created.");
        }

        if (m_different_transfer_queue)
        {
            vk::CommandPoolCreateInfo transfer_pool_info{};
            transfer_pool_info
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(m_queue_family_indices.m_transferFamily.value());

             auto createTransferCommandPoolResult = m_device.createCommandPool(transfer_pool_info);
             if (createTransferCommandPoolResult.result != vk::Result::eSuccess)
             {
                 BRR_LogError("Could not create transfer CommandPool! Result code: {}.", vk::to_string(createTransferCommandPoolResult.result).c_str());
                 exit(1);
             }
             m_transfer_command_pool = createTransferCommandPoolResult.value;

             BRR_LogInfo("Separate Transfer CommandPool created.");
        }
    }

    void VulkanRenderDevice::Init_Frames()
    {
        std::array<vk::CommandBuffer, FRAME_LAG> graphics_cmd_buffers;
        std::array<vk::CommandBuffer, FRAME_LAG> transfer_cmd_buffers;

        allocateCommandBuffer(m_device, m_graphics_command_pool, vk::CommandBufferLevel::ePrimary, FRAME_LAG, graphics_cmd_buffers.data());
        allocateCommandBuffer(m_device, m_transfer_command_pool, vk::CommandBufferLevel::ePrimary, FRAME_LAG, transfer_cmd_buffers.data());

        for (size_t idx = 0; idx < FRAME_LAG; ++idx)
        {
            m_frames[idx].graphics_cmd_buffer = graphics_cmd_buffers[idx];
            m_frames[idx].transfer_cmd_buffer = transfer_cmd_buffers[idx];

            // Render finished semaphores
            {
                auto createRenderFinishedSempahoreResult = m_device.createSemaphore(vk::SemaphoreCreateInfo{});
                if (createRenderFinishedSempahoreResult.result != vk::Result::eSuccess)
                {
                    BRR_LogError("Could not create Render Finished Semaphore for Frame structure! Result code: {}.", vk::to_string(createRenderFinishedSempahoreResult.result).c_str());
                    exit(1);
                }

                m_frames[idx].render_finished_semaphore = createRenderFinishedSempahoreResult.value;
            }
            // Transfer finished semaphores
            {
                auto createTransferFinishedSempahoreResult = m_device.createSemaphore(vk::SemaphoreCreateInfo{});
                if (createTransferFinishedSempahoreResult.result != vk::Result::eSuccess)
                {
                    BRR_LogError("Could not create Transfer Finished Semaphore for Frame structure! Result code: {}.", vk::to_string(createTransferFinishedSempahoreResult.result).c_str());
                    exit(1);
                }
                m_frames[idx].transfer_finished_semaphore = createTransferFinishedSempahoreResult.value;
            }
            // In flight fences
            {
                auto createInFlightFenceResult = m_device.createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
                if (createInFlightFenceResult.result != vk::Result::eSuccess)
                {
                    BRR_LogError("Could not create In Flight Fence for Frame structure! Result code: {}.", vk::to_string(createInFlightFenceResult.result).c_str());
                    exit(1);
                }
                m_frames[idx].in_flight_fences = createInFlightFenceResult.value;
            }
        }
    }

    void VulkanRenderDevice::Init_Texture2DSampler()
    {
        vk::SamplerCreateInfo sampler_create_info;
        sampler_create_info
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setAnisotropyEnable(VK_TRUE)
            .setMaxAnisotropy(m_device_properties.properties.limits.maxSamplerAnisotropy)
            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
            .setUnnormalizedCoordinates(VK_FALSE)
            .setCompareEnable(VK_FALSE)
            .setCompareOp(vk::CompareOp::eAlways)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setMipLodBias(0.0)
            .setMinLod(0.0)
            .setMaxLod(0.0);

        auto sampler_create_result = m_device.createSampler(sampler_create_info);
        if (sampler_create_result.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create Texture2D Sampler! Result code: {}", vk::to_string(sampler_create_result.result));
            exit(1);
        }

        m_texture2DSampler = sampler_create_result.value;
    }

    void VulkanRenderDevice::Free_FramePendingResources(Frame& frame)
    {
        for (auto& buffer_alloc_pair : frame.buffer_delete_list)
        {
            vmaDestroyBuffer(m_vma_allocator, buffer_alloc_pair.first, buffer_alloc_pair.second);
        }
        frame.buffer_delete_list.clear();

        for (auto& texture_alloc_info : frame.texture_delete_list)
        {
            m_device.destroyImageView(texture_alloc_info.image_view);
            vmaDestroyImage(m_vma_allocator, texture_alloc_info.image, texture_alloc_info.allocation);
        }
        frame.texture_delete_list.clear();
    }
}
