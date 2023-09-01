#ifndef BRR_STAGINALLOCATOR_H
#define BRR_STAGINALLOCATOR_H
#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/ResourcesHandles.h>

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

		StagingAllocator(StagingAllocator&& other) = delete;
		StagingAllocator(const StagingAllocator& other) = delete;
		StagingAllocator& operator=(const StagingAllocator& other) = delete;
		StagingAllocator& operator=(StagingAllocator&& other) = delete;

		~StagingAllocator();

		bool Init(VulkanRenderDevice* render_device);

		void DestroyAllocator();

        void AllocateStagingBuffer(size_t frame_id, size_t size,
                                                     StagingBufferHandle* out_staging_buffer);


        void WriteToStagingBuffer(StagingBufferHandle staging_buffer, uint32_t staging_buffer_offset, void* data, size_t data_size);


        void CopyFromStagingToBuffer(StagingBufferHandle staging_buffer, vk::Buffer dst_buffer,
                                     size_t size,
                                     uint32_t src_offset = 0, uint32_t dst_offset = 0);

    private:

		struct StagingBlock
		{
			size_t frame_id = 0;
			size_t m_filled_bytes = 0;

			void* mapping = nullptr;
			vk::Buffer m_buffer {};
			VmaAllocation m_allocation {};
		};

        bool InsertStagingBlock();
		void ClearProcessedBlocks(uint32_t current_frame_id);
		void AllocateInBlock(uint32_t block_index, size_t size, StagingBufferHandle* out_staging_handle);

		void InitVmaPool();
		void DestroystagingBlocks();

		VulkanRenderDevice* m_render_device = nullptr;

		VmaPool m_staging_pool{};

		uint32_t m_current_block = 0;
		std::vector<StagingBlock> m_staging_blocks {};

    };

}

#endif