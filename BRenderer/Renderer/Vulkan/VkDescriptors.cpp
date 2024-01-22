#include "VkDescriptors.h"
#include <Renderer/Vulkan/VkInitializerHelper.h>
#include <Renderer/Descriptors.h>

vk::DescriptorPool CreatePool(vk::Device device, const brr::render::DescriptorSetAllocator::PoolSizes& poolSizes, int count, vk::DescriptorPoolCreateFlags flags)
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
        BRR_LogError("Could not create DescriptorPool! Result code: {}.", vk::to_string(createDescriptorPoolResult.result).c_str());
        exit(1);
    }
    vk::DescriptorPool descriptorPool = createDescriptorPoolResult.value;

    return descriptorPool;
}

namespace brr::render
{
    //--------------------------------------------//
    //----------  DescriptorLayoutCache  ---------//
    //--------------------------------------------//

    size_t DescriptorLayoutCache::DescriptorLayoutHash::operator()(const DescriptorSetLayoutBindings& info) const 
    {
        using std::size_t;
        using std::hash;

        size_t result = hash<size_t>()(info.size());

        for (const vk::DescriptorSetLayoutBinding& b : info)
        {
            //pack the binding data into a single int64. Not fully correct but its ok
            size_t binding_hash = b.binding | static_cast<unsigned>(b.descriptorType) << 8 | (b.descriptorCount) << 16 | static_cast<unsigned>(b.stageFlags) << 24;

            //shuffle the packed binding data and xor it with the main hash
            result ^= hash<size_t>()(binding_hash);
        }

        return result;
    }

    DescriptorLayoutCache::DescriptorLayoutCache(vk::Device device)
    : m_device(device)
    {
    }

    DescriptorLayoutCache::~DescriptorLayoutCache()
    {
        Cleanup();
    }

    DescriptorLayoutHandle DescriptorLayoutCache::CreateDescriptorLayout(const DescriptorLayoutBindings& layout_bindings)
    {
        assert(IsValid() && "'CreateDescriptorLayout' called on invalid DescriptorLayoutCache");
        assert(!layout_bindings.m_bindings.empty() && "Can't create empty DescriptorSetLayout");

        std::vector<vk::DescriptorSetLayoutBinding> descriptor_bindings;
        descriptor_bindings.reserve(layout_bindings.m_bindings.size());

        bool isSorted = true;
        int32_t lastBinding = -1;
        for (uint32_t i = 0; i < layout_bindings.m_bindings.size(); i++)
        {
            if (static_cast<int32_t>(layout_bindings.m_bindings[i].binding) > lastBinding)
            {
                lastBinding = layout_bindings.m_bindings[i].binding;
            }
            else
            {
                isSorted = false;
            }
            vk::DescriptorSetLayoutBinding descriptor_set_layout_binding {};
            descriptor_set_layout_binding
                .setBinding(layout_bindings.m_bindings[i].binding)
                .setDescriptorType(VkHelpers::VkDescriptorTypeFromDescriptorType(layout_bindings.m_bindings[i].descriptor_type))
                .setStageFlags(VkHelpers::VkShaderStageFlagFromShaderStageFlag(layout_bindings.m_bindings[i].shader_stage_flag))
                .setDescriptorCount(1)
                .setPImmutableSamplers(nullptr);
            descriptor_bindings.push_back(descriptor_set_layout_binding);
        }

        if (!isSorted)
        {
            std::sort(descriptor_bindings.begin(), descriptor_bindings.end(),
            [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b)
            {
                return a.binding < b.binding;
            });
        }

        auto it = m_layoutCache.find(descriptor_bindings);
        if (it != m_layoutCache.end())
        {
            DescriptorLayoutHandle handle;
            handle.m_layout_index = (*it).second;
            return handle;
        }
        else
        {
            vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
            descriptor_set_layout_create_info
                .setBindings(descriptor_bindings);

            auto createDescSetLayoutResult = m_device.createDescriptorSetLayout(descriptor_set_layout_create_info);
            if (createDescSetLayoutResult.result != vk::Result::eSuccess)
            {
                BRR_LogError("Could not create DescriptorSetLayout! Result code: {}.", vk::to_string(createDescSetLayoutResult.result).c_str());
                exit(1);
            }
            vk::DescriptorSetLayout layout = createDescSetLayoutResult.value;
            m_descriptor_layouts_vector.emplace_back(layout);

            m_layoutCache[descriptor_bindings] = m_descriptor_layouts_vector.size() - 1;
            DescriptorLayoutHandle handle;
            handle.m_layout_index = m_descriptor_layouts_vector.size() - 1;
            return handle;
        }
    }

    vk::DescriptorSetLayout DescriptorLayoutCache::GetDescriptorLayout(DescriptorLayoutHandle descriptor_layout_handle) const
    {
        if (descriptor_layout_handle.m_layout_index >= m_descriptor_layouts_vector.size())
        {
            BRR_LogError("Invalid DescriptorLayoutHandle passed as parameter. Returning null VkDescriptorSetLayout.");
            return {};
        }
        return m_descriptor_layouts_vector[descriptor_layout_handle.m_layout_index];
    }

    void DescriptorLayoutCache::Cleanup()
    {
        for (auto& layout : m_descriptor_layouts_vector)
        {
            m_device.destroyDescriptorSetLayout(layout);
        }
        m_layoutCache.clear();
        m_descriptor_layouts_vector.clear();
    }

    //-----------------------------------------------//
    //-----------  DescriptorSetAllocator  ----------//
    //-----------------------------------------------//
    
    DescriptorSetAllocator::DescriptorSetAllocator(vk::Device device) : m_device(device)
    {
    }

    DescriptorSetAllocator::~DescriptorSetAllocator()
    {
        Cleanup();
    }

    void DescriptorSetAllocator::ResetPools()
    {
        m_set_pool_map.clear();
        for (auto pool : used_pools_)
        {
            m_device.resetDescriptorPool(pool);
            free_pools_.push_back(pool);
        }

        used_pools_.clear();

        current_pool_ = VK_NULL_HANDLE;
    }

    bool DescriptorSetAllocator::Allocate(const std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::DescriptorSet>& set)
    {
        assert(IsValid() && "'Allocate' called on invalid DescriptorSetAllocator");
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
            for (auto& desc_Set : set)
            {
                m_set_pool_map[desc_Set] = current_pool_;
            }
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
                for (auto& desc_Set : set)
                {
                    m_set_pool_map[desc_Set] = current_pool_;
                }
                return true;
            }

        default: 
            return false;
        }
    }

    bool DescriptorSetAllocator::Delete(vk::DescriptorSet descriptor_set)
    {
        auto pool_iter = m_set_pool_map.find(descriptor_set);
        if (pool_iter == m_set_pool_map.end())
        {
            BRR_LogError ("Trying to delete invalid DescriptorSet");
            return false;
        }

        const vk::DescriptorPool pool = pool_iter->second;
        m_set_pool_map.erase(descriptor_set);

        m_device.freeDescriptorSets(pool, descriptor_set);
        return true;
    }

    void DescriptorSetAllocator::Cleanup()
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

    vk::DescriptorPool DescriptorSetAllocator::GrabPool()
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
}