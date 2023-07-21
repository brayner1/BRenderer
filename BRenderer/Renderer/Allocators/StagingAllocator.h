#ifndef BRR_STAGINALLOCATOR_H
#define BRR_STAGINALLOCATOR_H
#include <vk_mem_alloc.h>

namespace brr::render
{
    class VulkanRenderDevice;
    class DeviceBuffer;

	class StagingBufferHandle
	{
	private:
		friend class StagingAllocator;
		size_t m_block_index;
		size_t m_offset;
		size_t m_size;
	};

    class StagingAllocator
    {
    public:

		StagingAllocator();

		~StagingAllocator();

        void AllocateStagingBuffer(size_t frame_id, size_t size,
                                                     StagingBufferHandle* out_staging_buffer);


        void WriteToStagingBuffer(StagingBufferHandle staging_buffer, uint32_t staging_buffer_offset, void* data, size_t data_size);


        void CopyFromStagingToBuffer(StagingBufferHandle staging_buffer, DeviceBuffer& dst_buffer,
                                     vk::CommandBuffer transfer_cmd_buffer, size_t size,
                                     uint32_t src_offset = 0, uint32_t dst_offset = 0);

    private:

		struct StagingBlock
		{
			size_t frame_id = 0;
			size_t m_filled_bytes = 0;
			void* mapping = nullptr;
			vk::Buffer m_buffer = VK_NULL_HANDLE;
			VmaAllocation m_allocation = VK_NULL_HANDLE;
		};

		void InsertStagingBlock();
		void ClearProcessedBlocks(uint32_t current_frame_id);
		void AllocateInBlock(uint32_t block_index, uint32_t size, StagingBufferHandle* out_staging_handle);

		void InitVmaPool();
		void DestroystagingBlocks();

		VulkanRenderDevice* m_render_device = nullptr;

		VmaPool m_staging_pool{};

		uint32_t m_current_block;
		std::vector<StagingBlock> m_staging_blocks;

    };

}

#endif