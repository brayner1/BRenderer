#ifndef BRR_DESCRIPTORS_H
#define BRR_DESCRIPTORS_H

namespace brr::render
{

	class DescriptorAllocator
	{
	public:
		explicit DescriptorAllocator(vk::Device device);

		void ResetPools();
		bool Allocate(vk::DescriptorSetLayout layout, vk::DescriptorSet* out_set);

		void Cleanup();


		struct PoolSizes
		{
			std::vector<std::pair<vk::DescriptorType, float>> sizes_multiplier
			{
				{ vk::DescriptorType::eSampler, 0.5f },
				{ vk::DescriptorType::eCombinedImageSampler, 4.f },
				{ vk::DescriptorType::eSampledImage, 4.f },
				{ vk::DescriptorType::eStorageImage, 1.f },
				{ vk::DescriptorType::eUniformTexelBuffer, 1.f },
				{ vk::DescriptorType::eStorageTexelBuffer, 1.f },
				{ vk::DescriptorType::eUniformBuffer, 2.f },
				{ vk::DescriptorType::eStorageBuffer, 2.f },
				{ vk::DescriptorType::eUniformBufferDynamic, 1.f },
				{ vk::DescriptorType::eStorageBufferDynamic, 1.f },
				{ vk::DescriptorType::eInputAttachment, 0.5f },
			};
		};

	private:

		vk::DescriptorPool GrabPool();

		vk::Device device_ {};

		vk::DescriptorPool current_pool_ {};
		PoolSizes descriptor_pool_sizes_;

		std::vector<vk::DescriptorPool> used_pools_ {};
		std::vector<vk::DescriptorPool> free_pools_ {};
	};


	class DescriptorLayoutCache
	{
	public:

		DescriptorLayoutCache(vk::Device device);

		vk::DescriptorSetLayout CreateDescriptorLayout(vk::DescriptorSetLayoutCreateInfo* layout_create_info);

		void Cleanup();

	private:

		struct DescriptorLayoutHash
		{
			size_t operator()()
			{
				
			}
		};

		
		vk::Device device;
	};

}

#endif