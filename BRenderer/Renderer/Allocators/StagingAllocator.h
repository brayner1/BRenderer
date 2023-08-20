#ifndef BRR_STAGINALLOCATOR_H
#define BRR_STAGINALLOCATOR_H
#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/ResourcesHandles.h>

#include "glm/vec2.hpp"

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

        uint32_t AllocateStagingBuffer(int64_t frame_id, size_t size,
                                       StagingBufferHandle* out_staging_buffer, bool can_segment = true);


        void WriteLinearBufferToStaging(StagingBufferHandle staging_buffer, uint32_t staging_buffer_offset, const void* data, size_t data_size);

        void WriteBlockImageToStaging(StagingBufferHandle staging_buffer, const unsigned char* image_data,
                                      glm::uvec2 block_offset, glm::uvec2 block_size, glm::uvec2 image_size, uint8_t
                                      pixel_size);


        void CopyFromStagingToBuffer(StagingBufferHandle staging_buffer, vk::Buffer dst_buffer,
                                     size_t size,
                                     uint32_t staging_buffer_offset = 0, uint32_t dst_buffer_offset = 0);

		void CopyFromStagingToImage(StagingBufferHandle staging_buffer, vk::Image dst_image,
                                    vk::Extent3D image_extent,
                                    uint32_t src_offset = 0, vk::Offset3D image_offset = 0);

    private:

		struct StagingBlock
		{
			int64_t frame_id = 0;
			size_t m_filled_bytes = 0;

			void* mapping = nullptr;
			vk::Buffer m_buffer {};
			VmaAllocation m_allocation {};
		};

		static constexpr uint32_t invalid_block = std::numeric_limits<uint32_t>::max();

        bool InsertStagingBlock();
		void ClearProcessedBlocks(int64_t current_frame_id);
        uint32_t AllocateInBlock(uint32_t block_index, size_t size, StagingBufferHandle* out_staging_handle);
		uint32_t FindAvailableBlock(int64_t frame_id, size_t size, bool can_segment);

		void InitVmaPool();
		void DestroystagingBlocks();

		VulkanRenderDevice* m_render_device = nullptr;

		VmaPool m_staging_pool{};

		uint32_t m_current_block = 0;
		std::vector<StagingBlock> m_staging_blocks {};

    };

}

#endif