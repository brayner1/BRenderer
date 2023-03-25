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

	auto createDescriptorPoolResult = device.createDescriptorPool(pool_info);
	if (createDescriptorPoolResult.result != vk::Result::eSuccess)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create DescriptorPool! Result code: %s.", vk::to_string(createDescriptorPoolResult.result).c_str());
		exit(1);
	}
	vk::DescriptorPool descriptorPool = createDescriptorPoolResult.value;

	return descriptorPool;
}

namespace brr::render
{
	//////////////////////////////
	//// Descriptor Allocator ////
	//////////////////////////////
	
	DescriptorAllocator::DescriptorAllocator(vk::Device device) : m_device(device)
	{
	}

	DescriptorAllocator::~DescriptorAllocator()
	{
		Cleanup();
	}

	void DescriptorAllocator::ResetPools()
	{
		for (auto pool : used_pools_)
		{
			m_device.resetDescriptorPool(pool);
			free_pools_.push_back(pool);
		}

		used_pools_.clear();

		current_pool_ = VK_NULL_HANDLE;
	}

	bool DescriptorAllocator::Allocate(const std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::DescriptorSet>& set)
	{
		assert(IsValid() && "'Allocate' called on invalid DescriptorAllocator");
		if (!current_pool_)
		{
			current_pool_ = GrabPool();
			used_pools_.push_back(current_pool_);
		}

		vk::DescriptorSetAllocateInfo allocate_info{};
		allocate_info
			.setSetLayouts(layouts)
			.setDescriptorPool(current_pool_);

		auto alloc_result = m_device.allocateDescriptorSets(allocate_info);

		switch (alloc_result.result)
		{
		case vk::Result::eSuccess:
			set = std::move(alloc_result.value);
			return true;

		case vk::Result::eErrorFragmentedPool:
		case vk::Result::eErrorOutOfPoolMemory:

			current_pool_ = GrabPool();
			used_pools_.push_back(current_pool_);

			allocate_info.setDescriptorPool(current_pool_);

			alloc_result = m_device.allocateDescriptorSets(allocate_info);

			if (alloc_result.result == vk::Result::eSuccess)
			{
				set = std::move(alloc_result.value);
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
			m_device.destroyDescriptorPool(pool);
		}
		free_pools_.clear();

		for (auto pool : used_pools_)
		{
			m_device.destroyDescriptorPool(pool);
		}
		used_pools_.clear();
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
			return CreatePool(m_device, descriptor_pool_sizes_, 1000, {});
		}
	}

	/////////////////////////////////
	//// Descriptor Layout Cache ////
	/////////////////////////////////

	DescriptorLayoutCache::DescriptorLayoutCache(vk::Device device)
	: m_device(device)
	{
	}

	vk::DescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(
		vk::DescriptorSetLayoutCreateInfo* layout_create_info)
	{
		assert(IsValid() && "'CreateDescriptorLayout' called on invalid DescriptorLayoutCache");
		DescriptorLayoutInfo layoutInfo;
		layoutInfo.m_bindings.reserve(layout_create_info->bindingCount);
		bool isSorted = true;
		int32_t lastBinding = -1;
		for (uint32_t i = 0; i < layout_create_info->bindingCount; i++)
		{
			layoutInfo.m_bindings.push_back(layout_create_info->pBindings[i]);

			if (static_cast<int32_t>(layout_create_info->pBindings[i].binding) > lastBinding)
			{
				lastBinding = layout_create_info->pBindings[i].binding;
			}
			else
			{
				isSorted = false;
			}
		}

		if (!isSorted)
		{
			std::sort(layoutInfo.m_bindings.begin(), layoutInfo.m_bindings.end(), 
			[](vk::DescriptorSetLayoutBinding& a, vk::DescriptorSetLayoutBinding& b)
			{
				return a.binding < b.binding;
			}
			);
		}

		auto it = m_layoutCache.find(layoutInfo);
		if (it != m_layoutCache.end())
		{
			return (*it).second;
		}
		else
		{
			vk::DescriptorSetLayout layout;
			auto createDescSetLayoutResult = m_device.createDescriptorSetLayout(*layout_create_info);
			if (createDescSetLayoutResult.result != vk::Result::eSuccess)
			{
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create DescriptorSetLayout! Result code: %s.", vk::to_string(createDescSetLayoutResult.result).c_str());
				exit(1);
			}
			layout = createDescSetLayoutResult.value;

			m_layoutCache[layoutInfo] = layout;
			return layout;
		}
	}

	void DescriptorLayoutCache::Cleanup()
	{
		for (auto layout_pair : m_layoutCache)
		{
			m_device.destroyDescriptorSetLayout(layout_pair.second);
		}
		m_layoutCache.clear();
	}

	bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
	{
		if (other.m_bindings.size() != m_bindings.size())
		{
			return false;
		}
		else {
			//compare each of the bindings is the same. Bindings are sorted so they will match
			for (int i = 0; i < m_bindings.size(); i++) {
				if (other.m_bindings[i].binding != m_bindings[i].binding)
				{
					return false;
				}
				if (other.m_bindings[i].descriptorType != m_bindings[i].descriptorType)
				{
					return false;
				}
				if (other.m_bindings[i].descriptorCount != m_bindings[i].descriptorCount)
				{
					return false;
				}
				if (other.m_bindings[i].stageFlags != m_bindings[i].stageFlags)
				{
					return false;
				}
			}
			return true;
		}
	}

	size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const
	{
		using std::size_t;
		using std::hash;

		size_t result = hash<size_t>()(m_bindings.size());

		for (const vk::DescriptorSetLayoutBinding& b : m_bindings)
		{
			//pack the binding data into a single int64. Not fully correct but its ok
			size_t binding_hash = b.binding | static_cast<unsigned>(b.descriptorType) << 8 | b.descriptorCount << 16 | static_cast<unsigned>(b.stageFlags) << 24;

			//shuffle the packed binding data and xor it with the main hash
			result ^= hash<size_t>()(binding_hash);
		}

		return result;
	}

	//--------------------------------------------//
	//--------  DescriptorLayoutBuilder  ---------//
	//--------------------------------------------//

	//! MakeDescriptorLayoutBuilder
	DescriptorLayoutBuilder DescriptorLayoutBuilder::MakeDescriptorLayoutBuilder(DescriptorLayoutCache* layoutCache)
	{
		DescriptorLayoutBuilder builder;

		builder.m_layoutCache = layoutCache;
		return builder;
	}

	//! SetBinding
	DescriptorLayoutBuilder& DescriptorLayoutBuilder::SetBinding(uint32_t binding, 
																 vk::DescriptorType type,
															 	 vk::ShaderStageFlags stageFlags)
	{
		vk::DescriptorSetLayoutBinding layoutBinding{};
		layoutBinding
			.setBinding(binding)
			.setDescriptorType(type)
			.setStageFlags(stageFlags)
			.setDescriptorCount(1)
			.setPImmutableSamplers(nullptr);

		m_layoutBindings.push_back(layoutBinding);

		return *this;
	}

	vk::DescriptorSetLayout DescriptorLayoutBuilder::GetDescriptorLayout() const
	{
		vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
		descriptor_set_layout_create_info
			.setBindings(m_layoutBindings);

		return m_layoutCache->CreateDescriptorLayout(&descriptor_set_layout_create_info);
	}
}
