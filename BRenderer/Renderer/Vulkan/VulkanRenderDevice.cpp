#include "VulkanRenderDevice.h"

#include <Renderer/RenderDefs.h>

#include <Visualization/Window.h>
#include <Core/LogSystem.h>
#include <Files/FilesUtils.h>
#include <Geometry/Geometry.h>

#include <filesystem>
#include <iostream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace brr::render
{
	static vk::BufferUsageFlags VkBufferUsageFromDeviceBufferUsage(VulkanRenderDevice::BufferUsage buffer_usage)
	{
		using BufferUsage = VulkanRenderDevice::BufferUsage;
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

	VmaMemoryUsage VmaMemoryUsageFromDeviceMemoryUsage(VulkanRenderDevice::MemoryUsage memory_usage)
	{
        switch (memory_usage)
        {
        case VulkanRenderDevice::AUTO:
			return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
        case VulkanRenderDevice::AUTO_PREFER_DEVICE:
			return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        case VulkanRenderDevice::AUTO_PREFER_HOST:
			return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        default:
			return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
        }
	}

	static vk::ShaderModule Create_ShaderModule(VulkanRenderDevice* device, std::vector<char>& code)
	{
		vk::ShaderModuleCreateInfo shader_module_info{};
		shader_module_info
			.setCodeSize(code.size())
			.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

		auto createShaderModuleResult = device->Get_VkDevice().createShaderModule(shader_module_info);
		if (createShaderModuleResult.result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not create ShaderModule! Result code: {}.", vk::to_string(createShaderModuleResult.result).c_str());
			return VK_NULL_HANDLE;
		}
		return createShaderModuleResult.value;
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
		}
		BRR_LogTrace("Destroyed frame sync semaphores.");

		m_staging_allocator.DestroyAllocator();
		BRR_LogTrace("Destroyed staging allocator.");

		vmaDestroyAllocator(m_vma_allocator);
		BRR_LogTrace("Destroyed VMA allocator.");

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
		Frame& current_frame = m_frames[m_current_buffer];

		Free_FramePendingResources(current_frame);

		if (!current_frame.graphics_cmd_buffer_begin)
		{
		    vk::Result graph_begin_result = BeginGraphicsCommandBuffer(current_frame.graphics_cmd_buffer);
			current_frame.graphics_cmd_buffer_begin = true;
		}

		if (!current_frame.transfer_cmd_buffer_begin)
		{
		    vk::Result transf_begin_result = BeginTransferCommandBuffer(current_frame.transfer_cmd_buffer);
			current_frame.transfer_cmd_buffer_begin = true;
		}

		BRR_LogTrace("Begin frame {}", m_current_frame);

		return m_current_frame;
    }

    vk::Semaphore VulkanRenderDevice::EndFrame(vk::Semaphore wait_semaphore, vk::Fence wait_fence)
    {
		Frame& current_frame = m_frames[m_current_buffer];

		current_frame.graphics_cmd_buffer.end();
		current_frame.transfer_cmd_buffer.end();

		current_frame.graphics_cmd_buffer_begin = false;
		current_frame.transfer_cmd_buffer_begin = false;

		vk::Result transfer_result = SubmitTransferCommandBuffers(1, &current_frame.transfer_cmd_buffer, 0, nullptr, nullptr, 1, &current_frame.transfer_finished_semaphore, nullptr);
	    BRR_LogTrace("Transfer command buffer submitted. Buffer: {:#x}. Frame {}. Buffer Index: {}", size_t(VkCommandBuffer((current_frame.transfer_cmd_buffer))), m_current_frame, m_current_buffer);

		std::array<vk::Semaphore, 2> wait_semaphores { wait_semaphore, current_frame.transfer_finished_semaphore };
		std::array<vk::PipelineStageFlags, 2> wait_stages { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eVertexInput };

		vk::Result result = SubmitGraphicsCommandBuffers(1, &current_frame.graphics_cmd_buffer, 2, wait_semaphores.data(), wait_stages.data(), 1, &current_frame.render_finished_semaphore, wait_fence);
		BRR_LogTrace("Graphics command buffer submitted. Buffer: {:#x}. Frame {}. Buffer Index: {}", size_t(VkCommandBuffer(current_frame.graphics_cmd_buffer)), m_current_frame, m_current_buffer);

		BRR_LogTrace("Frame ended. Frame {}. Buffer: {}", m_current_frame, m_current_buffer);

		++m_current_frame;
		m_current_buffer = (m_current_frame % FRAME_LAG);

		return current_frame.render_finished_semaphore;
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
		m_descriptor_allocator.reset(new DescriptorAllocator(m_device));

		BRR_LogInfo("VulkanRenderDevice {:#x} constructed", (size_t)this);
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
				BRR_LogWarn("Can't load shader file. '{}' is not a valid file path.", file_path.string());
				return Shader{};
			}

			std::vector<char> vertex_shader_code = files::ReadFile(file_path.string());

			vertex_shader_module = Create_ShaderModule(this, vertex_shader_code);
			if (!vertex_shader_module)
			{
			    BRR_LogError("Could not create vertex shader module. Shader file: '{}'", file_path.string());
				return Shader{};
			}

			shader.pipeline_stage_infos_.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(vertex_shader_module)
				.setPName("main"));

			BRR_LogTrace("Created vertex shader. Shader file: '{}'", file_path.string());
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
			if (!fragment_shader_module)
			{
			    BRR_LogError("Could not create fragment shader module. Shader file: '{}'", file_path.string());
				return Shader{};
			}

			shader.pipeline_stage_infos_.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(fragment_shader_module)
				.setPName("main"));

			BRR_LogTrace("Created fragment shader. Shader file: '{}'", file_path.string());
		}

		shader.m_isValid = true;
		shader.m_pDevice = this;
		shader.vert_shader_module_ = vertex_shader_module;
		shader.frag_shader_module_ = fragment_shader_module;

		shader.vertex_input_binding_description_ = Vertex3_PosColor::GetBindingDescription();
		shader.vertex_input_attribute_descriptions_ = Vertex3_PosColor::GetAttributeDescriptions();

		BRR_LogDebug("Created graphics shader object.\nVertex shader file:\t'{}'\nFragment shader file:\t'{}'", vertex_file_name, frag_file_name);

		return std::move(shader);
    }

    void VulkanRenderDevice::WaitIdle() const
    {
		BRR_LogTrace("Waiting for device idle.");
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

    vk::CommandBuffer VulkanRenderDevice::GetCurrentGraphicsCommandBuffer()
    {
		Frame& current_frame = m_frames[m_current_buffer];

		return current_frame.graphics_cmd_buffer;
    }

    vk::CommandBuffer VulkanRenderDevice::GetCurrentTransferCommandBuffer()
    {
		Frame& current_frame = m_frames[m_current_buffer];

		return current_frame.transfer_cmd_buffer;
    }

    DescriptorLayoutBuilder VulkanRenderDevice::GetDescriptorLayoutBuilder() const
    {
		return DescriptorLayoutBuilder::MakeDescriptorLayoutBuilder(m_descriptor_layout_cache.get());
    }

    DescriptorSetBuilder<FRAME_LAG> VulkanRenderDevice::GetDescriptorSetBuilder(
        const DescriptorLayout& layout) const
    {
		return DescriptorSetBuilder<FRAME_LAG>::MakeDescriptorSetBuilder(layout, m_descriptor_allocator.get());
    }

    BufferHandle VulkanRenderDevice::CreateBuffer(size_t buffer_size, BufferUsage buffer_usage,
                                                   MemoryUsage memory_usage,
                                                   VmaAllocationCreateFlags buffer_allocation_flags)
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

			VmaMemoryUsage vma_memory_usage = VmaMemoryUsageFromDeviceMemoryUsage(memory_usage);

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = vma_memory_usage;
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

		Frame& current_frame = m_frames[m_current_buffer];
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

		BRR_LogDebug("Mapped buffer. Buffer: {:#x}. Mapped adress: {:#x}.", size_t(VkBuffer(buffer->buffer)), (size_t)buffer->mapped);

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

		BRR_LogDebug("Unmapped buffer. Buffer: {:#x}.", size_t(VkBuffer(buffer->buffer)));
    }

    bool VulkanRenderDevice::UploadBufferData(BufferHandle dst_buffer_handle, void* data, size_t size, uint32_t offset)
    {
		render::StagingBufferHandle staging_buffer{};
		m_staging_allocator.AllocateStagingBuffer(m_current_frame, size, &staging_buffer);

		m_staging_allocator.WriteToStagingBuffer(staging_buffer, 0, data, size);

		Buffer* dst_buffer = m_buffer_alloc.GetResource(dst_buffer_handle);

		m_staging_allocator.CopyFromStagingToBuffer(staging_buffer, dst_buffer->buffer, size);

		return true;
    }

    bool VulkanRenderDevice::CopyBuffer(BufferHandle src_buffer_handle, BufferHandle dst_buffer_handle, size_t size,
                                        uint32_t src_buffer_offset, uint32_t dst_buffer_offset, bool use_transfer_queue)
    {
		Frame& current_frame = m_frames[m_current_buffer];

		vk::CommandBuffer cmd_buffer = use_transfer_queue? current_frame.transfer_cmd_buffer : current_frame.graphics_cmd_buffer;

		Buffer* src_buffer = m_buffer_alloc.GetResource(src_buffer_handle);
		Buffer* dst_buffer = m_buffer_alloc.GetResource(dst_buffer_handle);

		vk::BufferCopy copy_region{};
		copy_region
			.setSrcOffset(src_buffer_offset)
			.setDstOffset(dst_buffer_offset)
			.setSize(size);

		cmd_buffer.copyBuffer(src_buffer->buffer, dst_buffer->buffer, copy_region);

		//TODO: Check if src buffer is on CPU or GPU to decide if uses transfer queue or no.
		if (use_transfer_queue)
		{
		    {
		        vk::BufferMemoryBarrier2 src_buffer_memory_barrier;
		        src_buffer_memory_barrier
                    .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                    .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                    .setSrcQueueFamilyIndex(GetQueueFamilyIndices().m_transferFamily.value())
                    .setDstQueueFamilyIndex(GetQueueFamilyIndices().m_graphicsFamily.value())
                    .setBuffer(dst_buffer->buffer)
                    .setSize(size);

		        vk::DependencyInfo dependency_info;
		        dependency_info
                    .setBufferMemoryBarriers(src_buffer_memory_barrier);

		        current_frame.transfer_cmd_buffer.pipelineBarrier2(dependency_info);
		    }

		    {
		        vk::BufferMemoryBarrier2 dst_buffer_memory_barrier {};
		        dst_buffer_memory_barrier
                    .setBuffer(dst_buffer->buffer)
                    .setSize(size)
                    .setSrcQueueFamilyIndex(GetQueueFamilyIndices().m_transferFamily.value())
                    .setDstQueueFamilyIndex(GetQueueFamilyIndices().m_graphicsFamily.value())
                    .setDstStageMask(vk::PipelineStageFlagBits2::eVertexAttributeInput)
                    .setDstAccessMask(vk::AccessFlagBits2::eVertexAttributeRead);

		        vk::DependencyInfo dependency_info {};
		        dependency_info
                    .setBufferMemoryBarriers(dst_buffer_memory_barrier);

		        current_frame.graphics_cmd_buffer.pipelineBarrier2(dependency_info);
		    }
		}
		else
		{
			//TODO: Select DstAccessMask and DstStageMask based on buffer usage.
		    vk::MemoryBarrier memory_barrier;
            memory_barrier
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead);

            current_frame.graphics_cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                                              vk::PipelineStageFlagBits::eVertexInput,
                                                              vk::DependencyFlags(), 1, &memory_barrier, 0,
                                                              nullptr, 0, nullptr);
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

		GetTransferQueue().waitIdle();

		m_device.freeCommandBuffers(transfer_cmd_pool, cmd_buffer);

		BRR_LogDebug("Immeadite copy buffers. Src buffer: {:#x}. Dst Buffer: {:#x}. Copy size: {}", size_t(VkBuffer(src_buffer->buffer)), size_t(VkBuffer(dst_buffer->buffer)), size);

		return true;
    }

    vk::DescriptorBufferInfo VulkanRenderDevice::GetBufferDescriptorInfo(BufferHandle buffer_handle, vk::DeviceSize size, vk::DeviceSize offset)
    {
		Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);

		return vk::DescriptorBufferInfo{buffer->buffer, offset, size};
    }

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

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = vma_memory_usage;
			allocInfo.flags = 0;
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
	        UpdateBufferData(vertex_buffer->buffer, data, buffer_size, 0, 0);
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

		Frame& current_frame = m_frames[m_current_buffer];
		current_frame.buffer_delete_list.emplace_back(vertex_buffer->buffer, vertex_buffer->buffer_allocation);

		BRR_LogDebug("Destroyed vertex buffer. Buffer: {:#x}", (size_t)(static_cast<VkBuffer>(vertex_buffer->buffer)));

		return m_vertex_buffer_alloc.DestroyResource(vertex_buffer_handle);
    }

    bool VulkanRenderDevice::UpdateVertexBufferData(VertexBufferHandle vertex_buffer_handle, void* data, size_t data_size, uint32_t dst_offset)
    {
        const VertexBuffer* vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
		if (!vertex_buffer)
		{
		    return false;
		}

		UpdateBufferData(vertex_buffer->buffer, data, data_size, 0, dst_offset);

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
			assert(index_buffer && "VertexBuffer not initialized. Something is very wrong.");

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

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = vma_memory_usage;
			allocInfo.flags = 0;
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
				m_index_buffer_alloc.DestroyResource(index_buffer_handle);
				BRR_LogError("Could not create VertexBuffer! Result code: {}.", vk::to_string(createBufferResult).c_str());
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
			UpdateBufferData(index_buffer->buffer, data, buffer_size, 0, 0);
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

		Frame& current_frame = m_frames[m_current_buffer];
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

    void VulkanRenderDevice::UpdateBufferData(vk::Buffer dst_buffer, void* data, size_t size,
                                              uint32_t src_offset, uint32_t dst_offset)
    {
		render::StagingBufferHandle staging_buffer{};
		m_staging_allocator.AllocateStagingBuffer(m_current_frame, size, &staging_buffer);

		m_staging_allocator.WriteToStagingBuffer(staging_buffer, 0, data, size);

		m_staging_allocator.CopyFromStagingToBuffer(staging_buffer, dst_buffer, size, src_offset, dst_offset);

		vk::CommandBuffer transfer_cmd_buffer = GetCurrentTransferCommandBuffer();
		vk::CommandBuffer grapics_cmd_buffer = GetCurrentGraphicsCommandBuffer();

		if (IsDifferentTransferQueue())
        {
            vk::BufferMemoryBarrier2 buffer_memory_barrier;
            buffer_memory_barrier
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setSrcQueueFamilyIndex(GetQueueFamilyIndices().m_transferFamily.value())
                .setDstQueueFamilyIndex(GetQueueFamilyIndices().m_graphicsFamily.value())
                .setBuffer(dst_buffer)
                .setSize(size);

            vk::DependencyInfo dependency_info;
            dependency_info
                .setBufferMemoryBarriers(buffer_memory_barrier);

            transfer_cmd_buffer.pipelineBarrier2(dependency_info);

            buffer_memory_barrier
                .setDstStageMask(vk::PipelineStageFlagBits2::eVertexInput)
                .setDstAccessMask(vk::AccessFlagBits2::eVertexAttributeRead)
                .setSrcQueueFamilyIndex(GetQueueFamilyIndices().m_transferFamily.value())
                .setDstQueueFamilyIndex(GetQueueFamilyIndices().m_graphicsFamily.value())
                .setBuffer(dst_buffer)
                .setSize(size);

            dependency_info
                .setBufferMemoryBarriers(buffer_memory_barrier);

            grapics_cmd_buffer.pipelineBarrier2(dependency_info);
        }
        else
        {
            // TODO
            vk::MemoryBarrier memory_barrier;
            memory_barrier
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead);

            transfer_cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                                vk::PipelineStageFlagBits::eVertexInput,
                                                vk::DependencyFlags(), 1, &memory_barrier, 0,
                                                nullptr, 0, nullptr);
        }
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
		            BRR_LogError("Could not create Render Finished Semaphore for swapchain! Result code: {}.", vk::to_string(createRenderFinishedSempahoreResult.result).c_str());
		            exit(1);
		        }

		        m_frames[idx].render_finished_semaphore = createRenderFinishedSempahoreResult.value;
		    }
			// Transfer finished semaphores
		    {
		        auto createTransferFinishedSempahoreResult = m_device.createSemaphore(vk::SemaphoreCreateInfo{});
		        if (createTransferFinishedSempahoreResult.result != vk::Result::eSuccess)
		        {
		            BRR_LogError("Could not create Transfer Finished Semaphore for swapchain! Result code: {}.", vk::to_string(createTransferFinishedSempahoreResult.result).c_str());
		            exit(1);
		        }
		        m_frames[idx].transfer_finished_semaphore = createTransferFinishedSempahoreResult.value;
		    }
		}
    }

    vk::Result VulkanRenderDevice::BeginGraphicsCommandBuffer(vk::CommandBuffer graphics_cmd_buffer)
    {
		const vk::Result graph_reset_result = graphics_cmd_buffer.reset();
		if (graph_reset_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not reset current graphics command buffer of frame {}. Result code: {}", m_current_frame, vk::to_string(graph_reset_result).c_str());
			return graph_reset_result;
		}

        const vk::CommandBufferBeginInfo cmd_buffer_begin_info{};
		return graphics_cmd_buffer.begin(cmd_buffer_begin_info);
    }

    vk::Result VulkanRenderDevice::BeginTransferCommandBuffer(vk::CommandBuffer transfer_cmd_buffer)
    {
		const vk::Result transf_reset_result = transfer_cmd_buffer.reset();
		if (transf_reset_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not reset current transfer command buffer of frame {}. Result code: {}", m_current_frame, vk::to_string(transf_reset_result).c_str());
			return transf_reset_result;
		}

		const vk::CommandBufferBeginInfo cmd_buffer_begin_info{};
		return transfer_cmd_buffer.begin(cmd_buffer_begin_info);
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

    void VulkanRenderDevice::Free_FramePendingResources(Frame& frame)
    {
		for (auto& buffer_alloc_pair : frame.buffer_delete_list)
		{
		    vmaDestroyBuffer(m_vma_allocator, buffer_alloc_pair.first, buffer_alloc_pair.second);
		}
		frame.buffer_delete_list.clear();
    }
}
