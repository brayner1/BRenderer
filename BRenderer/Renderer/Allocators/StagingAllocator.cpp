#include "StagingAllocator.h"

#include <Renderer/DeviceBuffer.h>
#include <Renderer/RenderDefs.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>

namespace brr::render
{
    constexpr uint32_t STAGING_BLOCK_SIZE_BYTES = STAGING_BLOCK_SIZE_KB * 1024;
    constexpr uint32_t STAGING_MAX_SIZE_BYTES = STAGING_BUFFER_MAX_SIZE_MB * 1024 * 1024;

    StagingAllocator::StagingAllocator()
    {}

    StagingAllocator::~StagingAllocator()
    {
        DestroyAllocator();
    }

    bool StagingAllocator::Init(VulkanRenderDevice* render_device)
    {
        m_render_device = render_device;
        BRR_LogInfo("Initializing StagingAllocator.");
        InitVmaPool();
        m_current_block = 0;
        for (size_t i = 0; i < FRAME_LAG; i++)
        {
            InsertStagingBlock();
        }

        return true;
    }

    void StagingAllocator::DestroyAllocator()
    {
        BRR_LogInfo("Destroying StagingAllocator.");
        DestroystagingBlocks();
        if (m_staging_pool != VK_NULL_HANDLE)
        {
            vmaDestroyPool(m_render_device->m_vma_allocator, m_staging_pool);
            m_staging_pool = VK_NULL_HANDLE;
        }
    }

    void StagingAllocator::AllocateStagingBuffer(size_t frame_id, size_t size, StagingBufferHandle* out_staging_buffer)
    {
        assert(out_staging_buffer && "StagingBufferHandle pointer must be valid to allocate a new staging buffer.");
        if (m_staging_blocks[m_current_block].frame_id == frame_id)
        {
            // Current block is used by this frame. Check if there is enough space.
            if (m_staging_blocks[m_current_block].m_filled_bytes + size < STAGING_BLOCK_SIZE_BYTES)
            {
                // Block has enough space. Allocate memory in it.
                AllocateInBlock(m_current_block, size, out_staging_buffer);
            }
            else
            {
                // Not enough space in current block. Find another one.
                bool found = false;
                for (uint32_t i = 0; i < m_staging_blocks.size(); i++)
                {
                    StagingBlock& block = m_staging_blocks[i];
                    if (block.frame_id <= frame_id - FRAME_LAG)
                    {
                        // Clear already processed staging buffers
                        block.m_filled_bytes = 0;
                        block.frame_id = 0;
                        if (!found)
                        {
                            found = true;
                            m_current_block = i;
                            block.frame_id = frame_id;
                        }
                    }
                    else if (block.frame_id == frame_id && (block.m_filled_bytes + size) < STAGING_BLOCK_SIZE_BYTES)
                    {
                        // Block already used in this frame still has free space. Use it.
                        if (!found)
                        {
                            found = true;
                            m_current_block = i;
                        }
                    }
                }
                if (!found)
                {
                    // Problem here. There is no block available. Let's try to create a new one.
                    if (((m_staging_blocks.size() + 1) * STAGING_BLOCK_SIZE_BYTES) <= STAGING_MAX_SIZE_BYTES)
                    {
                        InsertStagingBlock();
                        m_current_block = m_staging_blocks.size() - 1;
                    }
                    else
                    {
                        // There is not enough space to allocate new block.
                        // We need to wait for previous frames to finish processing.
                        m_render_device->WaitIdle(); // TODO: is this the best approach?
                        for (uint32_t i = 0; i < m_staging_blocks.size(); i++)
                        {
                            StagingBlock& block = m_staging_blocks[i];
                            if (block.frame_id != frame_id)
                            {
                                // Clear already processed staging buffers
                                block.m_filled_bytes = 0;
                                block.frame_id = 0;
                                if (!found)
                                {
                                    found = true;
                                    m_current_block = i;
                                    block.frame_id = frame_id;
                                }
                            }
                        }
                    }
                }
                // Allocate memory in new block.
                AllocateInBlock(m_current_block, size, out_staging_buffer);
            }
        }
        else if (m_staging_blocks[m_current_block].frame_id <= frame_id - FRAME_LAG)
        {
            // Clear the old block and reuse it.
            StagingBlock& block = m_staging_blocks[m_current_block];
            block.m_filled_bytes = 0;
            block.frame_id = frame_id;
            AllocateInBlock(m_current_block, size, out_staging_buffer);
        }
        else
        {
            bool found = false;
            for (uint32_t i = 0; i < m_staging_blocks.size(); i++)
            {
                StagingBlock& block = m_staging_blocks[i];
                if (block.frame_id <= frame_id - FRAME_LAG)
                {
                    // Clear already processed staging buffers
                    block.m_filled_bytes = 0;
                    block.frame_id = 0;
                    if (!found)
                    {
                        found = true;
                        m_current_block = i;
                        block.frame_id = frame_id;
                    }
                }
                else if (block.frame_id == frame_id && (block.m_filled_bytes + size) < STAGING_BLOCK_SIZE_BYTES)
                {
                    // Block already used in this frame still has free space. Use it.
                    if (!found)
                    {
                        found = true;
                        m_current_block = i;
                    }
                }
            }
            if (!found)
            {
                // Problem here. There is no block available. Let's try to create a new one.
                if (((m_staging_blocks.size() + 1) * STAGING_BLOCK_SIZE_BYTES) <= STAGING_MAX_SIZE_BYTES)
                {
                    InsertStagingBlock();
                    m_current_block = m_staging_blocks.size() - 1;
                }
                else
                {
                    // There is not enough space to allocate new block.
                    // We need to wait for previous frames to finish processing.
                    m_render_device->WaitIdle(); // TODO: is this the best approach?
                    for (uint32_t i = 0; i < m_staging_blocks.size(); i++)
                    {
                        StagingBlock& block = m_staging_blocks[i];
                        if (block.frame_id != frame_id)
                        {
                            // Clear already processed staging buffers
                            block.m_filled_bytes = 0;
                            block.frame_id = 0;
                            if (!found)
                            {
                                found = true;
                                m_current_block = i;
                                block.frame_id = frame_id;
                            }
                        }
                    }
                }
            }
            AllocateInBlock(m_current_block, size, out_staging_buffer);
        }
    }

    void StagingAllocator::WriteToStagingBuffer(StagingBufferHandle staging_buffer, uint32_t staging_buffer_offset, void* data, size_t data_size)
    {
        StagingBlock& block = m_staging_blocks[staging_buffer.m_block_index];
        memcpy(static_cast<char*>(block.mapping) + staging_buffer.m_offset + staging_buffer_offset, data, data_size);
    }

    void StagingAllocator::CopyFromStagingToBuffer(StagingBufferHandle staging_buffer, vk::Buffer dst_buffer,
                                                   size_t size,
                                                   uint32_t src_offset, uint32_t dst_offset)
    {
        assert((dst_offset + size) <= staging_buffer.m_size && "Can't make transfer bigger than passed staging buffer.");
        StagingBlock& block = m_staging_blocks[staging_buffer.m_block_index];

        vk::CommandBuffer transfer_cmd_buffer = m_render_device->GetCurrentTransferCommandBuffer();
        
        vk::BufferCopy buffer_copy;
        buffer_copy
            .setSrcOffset(staging_buffer.m_offset + src_offset)
            .setDstOffset(dst_offset)
            .setSize(size);
        transfer_cmd_buffer.copyBuffer(block.m_buffer, dst_buffer, buffer_copy);
    }

    bool StagingAllocator::InsertStagingBlock()
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info
            .setSize(STAGING_BLOCK_SIZE_BYTES)
            .setUsage(vk::BufferUsageFlagBits::eTransferSrc);

        VmaAllocationCreateInfo allocation_create_info;
        allocation_create_info.pool = m_staging_pool;
        allocation_create_info.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo alloc_info;

        StagingBlock block;
        VkBuffer new_buffer;
        //TODO: Handle error
        VkResult result = vmaCreateBuffer(m_render_device->m_vma_allocator,
                                          reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info),
                                          &allocation_create_info,
                                          &new_buffer,
                                          &block.m_allocation,
                                          &alloc_info);
        if (result != VK_SUCCESS)
        {
            BRR_LogError("Error allocating staging block of size: {} bytes;", STAGING_BLOCK_SIZE_BYTES);
            return false;
        }


        block.m_buffer = new_buffer;
        block.mapping = alloc_info.pMappedData;

        m_staging_blocks.push_back(block);

        BRR_LogInfo("Allocated staging block. Buffer: {:#x}, Size: {}", (size_t)new_buffer, STAGING_BLOCK_SIZE_BYTES);

        return true;
    }

    void StagingAllocator::ClearProcessedBlocks(uint32_t current_frame_id)
    {
        for (StagingBlock& block : m_staging_blocks)
        {
            if (block.frame_id <= (current_frame_id - FRAME_LAG))
            {
                // Block used in previous frame, already processed.
                block.frame_id = current_frame_id;
                block.m_filled_bytes = 0;
            }
        }
    }

    void StagingAllocator::AllocateInBlock(uint32_t block_index, size_t size, StagingBufferHandle* out_staging_handle)
    {
        out_staging_handle->m_block_index = block_index;
        out_staging_handle->m_offset = m_staging_blocks[block_index].m_filled_bytes;
        out_staging_handle->m_size = size;

        m_staging_blocks[block_index].m_filled_bytes += size;

        BRR_LogInfo("Allocating {} bytes in staging block {}.", size, block_index);
    }

    void StagingAllocator::InitVmaPool()
    {
        vk::BufferCreateInfo sample_buff_create_info;
        sample_buff_create_info
            .setSize(1024)
            .setUsage(vk::BufferUsageFlagBits::eTransferSrc);

        VmaAllocationCreateInfo sample_alloc_info;
        sample_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        sample_alloc_info.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        sample_alloc_info.requiredFlags = 0;
        sample_alloc_info.preferredFlags = 0;
        sample_alloc_info.memoryTypeBits = 0;
        sample_alloc_info.pool = VK_NULL_HANDLE;
        sample_alloc_info.pUserData = VK_NULL_HANDLE;
        sample_alloc_info.priority = 1.0;


        uint32_t mem_type_index;
        VkResult result = vmaFindMemoryTypeIndexForBufferInfo(m_render_device->m_vma_allocator,
                                                              reinterpret_cast<VkBufferCreateInfo*>
                                                              (&sample_buff_create_info),
                                                              &sample_alloc_info, &mem_type_index);

        BRR_LogInfo("Find memory type index result: {}", vk::to_string(static_cast<vk::Result>(result)));

        VmaPoolCreateInfo pool_create_info;
        pool_create_info.memoryTypeIndex = mem_type_index;
        pool_create_info.blockSize = STAGING_BLOCK_SIZE_BYTES;
        pool_create_info.maxBlockCount = (STAGING_MAX_SIZE_BYTES) / (STAGING_BLOCK_SIZE_BYTES);
        pool_create_info.minBlockCount = FRAME_LAG;
        pool_create_info.flags = 0;
        pool_create_info.pMemoryAllocateNext = nullptr;
        pool_create_info.minAllocationAlignment = 0;
        pool_create_info.priority = 1.0;

        vmaCreatePool(m_render_device->m_vma_allocator, &pool_create_info, &m_staging_pool);

        BRR_LogInfo("StagingAllocator VmaPool is created.\nPool properties:\n"
                    "\tMemory type index: {}\n"
                    "\tPool Max Size (Bytes): {}\n"
                    "\tPool Current Size (Bytes): {}\n"
                    "\tBlock Size (Bytes): {}\n", mem_type_index, STAGING_MAX_SIZE_BYTES,
                    FRAME_LAG * STAGING_BLOCK_SIZE_BYTES, STAGING_BLOCK_SIZE_BYTES);
    }

    void StagingAllocator::DestroystagingBlocks()
    {
        for (StagingBlock& block : m_staging_blocks)
        {
            BRR_LogInfo("Destroying staging block. Buffer: {:#x}", (size_t)((VkBuffer)block.m_buffer));
            vmaDestroyBuffer(m_render_device->m_vma_allocator, block.m_buffer, block.m_allocation);
        }

        m_staging_blocks.clear();
    }
}
