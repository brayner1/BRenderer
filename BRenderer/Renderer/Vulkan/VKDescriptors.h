#ifndef BRR_VKDESCRIPTORS_H
#define BRR_VKDESCRIPTORS_H
#include <Renderer/Vulkan/VulkanInc.h>

#include <unordered_map>

namespace brr::render
{
    struct DescriptorLayoutHandle;
    struct DescriptorLayoutBindings;

    //--------------------------------------------//
    //----------  DescriptorLayoutCache  ---------//
    //--------------------------------------------//
    class DescriptorLayoutCache
    {
    public:
        DescriptorLayoutCache() = default;
        explicit DescriptorLayoutCache(vk::Device device);

        ~DescriptorLayoutCache();

        DescriptorLayoutHandle CreateDescriptorLayout(const DescriptorLayoutBindings& layout_bindings);

        vk::DescriptorSetLayout GetDescriptorLayout(DescriptorLayoutHandle descriptor_layout_handle) const;

        void Cleanup();

        [[nodiscard]] bool IsValid() const { return m_device; }

    private:
        using DescriptorSetLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>;
        using DescriptorLayoutIndex = uint32_t;

        struct DescriptorLayoutHash
        {
            size_t operator()(const DescriptorSetLayoutBindings& info) const;
        };

        std::unordered_map<DescriptorSetLayoutBindings, DescriptorLayoutIndex, DescriptorLayoutHash> m_layoutCache;
        std::vector<vk::DescriptorSetLayout> m_descriptor_layouts_vector;
        vk::Device m_device {};
    };

    //-----------------------------------------------//
    //-----------  DescriptorSetAllocator  ----------//
    //-----------------------------------------------//
    class DescriptorSetAllocator
    {
    public:
        DescriptorSetAllocator() = default;
        explicit DescriptorSetAllocator(vk::Device device);

        ~DescriptorSetAllocator();

        void ResetPools();
        bool Allocate(const std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::DescriptorSet>& out_set);
        bool Delete(vk::DescriptorSet descriptor_set);

        void Cleanup();

        [[nodiscard]] vk::Device GetDevice() const { return m_device; }

        [[nodiscard]] bool IsValid() const { return m_device; }

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

        vk::Device m_device {};

        vk::DescriptorPool current_pool_ {};
        PoolSizes descriptor_pool_sizes_;

        std::vector<vk::DescriptorPool> used_pools_ {};
        std::vector<vk::DescriptorPool> free_pools_ {};

        std::unordered_map<vk::DescriptorSet, vk::DescriptorPool> m_set_pool_map {};
    };
}

#endif
