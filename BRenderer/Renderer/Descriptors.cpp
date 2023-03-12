#include "Renderer/Descriptors.h"

vk::DescriptorPool CreatePool(vk::Device device, const brr::render::DescriptorAllocator::PoolSizes& poolSizes, int count, vk::DescriptorPoolCreateFlags flags)
{
	std::vector<vk::DescriptorPoolSize> sizes;
	sizes.reserve(poolSizes.sizes_multiplier.size());
	for (auto sz : poolSizes.sizes_multiplier) {
		sizes.emplace_back(sz.first, uint32_t(sz.second * count));
	}
	vk::DescriptorPoolCreateInfo pool_info = {};
	pool_info
		.setFlags(flags)
		.setMaxSets(count)
		.setPoolSizes(sizes);

	vk::DescriptorPool descriptorPool = device.createDescriptorPool(pool_info);

	return descriptorPool;
}

namespace brr::render
{
	//////////////////////////////
	//// Descriptor Allocator ////
	//////////////////////////////
	
	DescriptorAllocator::DescriptorAllocator(vk::Device device) : device_(device)
	{
	}

	void DescriptorAllocator::ResetPools()
	{
		for (auto pool : used_pools_)
		{
			device_.resetDescriptorPool(pool);
			free_pools_.push_back(pool);
		}

		used_pools_.clear();

		current_pool_ = VK_NULL_HANDLE;
	}

	bool DescriptorAllocator::Allocate(vk::DescriptorSetLayout layout, vk::DescriptorSet* set)
	{
		if (!current_pool_)
		{
			current_pool_ = GrabPool();
			used_pools_.push_back(current_pool_);
		}

		vk::DescriptorSetAllocateInfo allocate_info{};
		allocate_info
			.setSetLayouts(layout)
			.setDescriptorPool(current_pool_)
			.setDescriptorSetCount(1);

		vk::Result result = device_.allocateDescriptorSets(&allocate_info, set);

		switch (result)
		{
		case vk::Result::eSuccess:
			return true;

		case vk::Result::eErrorFragmentedPool:
		case vk::Result::eErrorOutOfPoolMemory:

			current_pool_ = GrabPool();
			used_pools_.push_back(current_pool_);

			allocate_info.setDescriptorPool(current_pool_);

			result = device_.allocateDescriptorSets(&allocate_info, set);

			if (result == vk::Result::eSuccess)
			{
				return true;
			}

		default: 
			return false;
		}
	}

	void DescriptorAllocator::Cleanup()
	{
		for (auto pool : free_pools_)
		{
			device_.destroyDescriptorPool(pool);
		}

		for (auto pool : used_pools_)
		{
			device_.destroyDescriptorPool(pool);
		}
	}

	vk::DescriptorPool DescriptorAllocator::GrabPool()
	{
		if (free_pools_.size() > 0)
		{
			vk::DescriptorPool pool = free_pools_.back();
			free_pools_.pop_back();

			return pool;
		}
		else
		{
			return CreatePool(device_, descriptor_pool_sizes_, 1000, {});
		}
	}

	/////////////////////////////////
	//// Descriptor Layout Cache ////
	/////////////////////////////////
}
