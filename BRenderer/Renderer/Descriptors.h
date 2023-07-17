#ifndef BRR_DESCRIPTORS_H
#define BRR_DESCRIPTORS_H

#include "Core/LogSystem.h"

namespace brr::render
{
    //--------------------------------------------//
    //-----------  DescriptorAllocator  ----------//
    //--------------------------------------------//
    class DescriptorAllocator
    {
    public:
        DescriptorAllocator() = default;
        explicit DescriptorAllocator(vk::Device device);

        ~DescriptorAllocator();

        void ResetPools();
        bool Allocate(const std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::DescriptorSet>& out_set);

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
    };

    //--------------------------------------------//
    //--------  DescriptorLayoutBindings  --------//
    //--------------------------------------------//
    struct DescriptorLayoutBindings
    {
        DescriptorLayoutBindings() = default;

        bool operator==(const DescriptorLayoutBindings& other) const;

        vk::DescriptorSetLayoutBinding& operator [] (uint32_t binding) { return m_bindings[binding]; }

        size_t Hash() const;

        std::vector<vk::DescriptorSetLayoutBinding> m_bindings {};
    };

    //--------------------------------------------//
    //------------  DescriptorLayout  ------------//
    //--------------------------------------------//
    struct DescriptorLayout
    {
        vk::DescriptorSetLayout  m_descriptor_set_layout;
        DescriptorLayoutBindings m_bindings;
    };

    //--------------------------------------------//
    //----------  DescriptorLayoutCache  ---------//
    //--------------------------------------------//
    class DescriptorLayoutCache
    {
    public:
        DescriptorLayoutCache() = default;
        explicit DescriptorLayoutCache(vk::Device device);

        ~DescriptorLayoutCache();

        vk::DescriptorSetLayout CreateDescriptorLayout(DescriptorLayoutBindings layout_bindings);

        void Cleanup();

        [[nodiscard]] bool IsValid() const { return m_device; }

    private:

        struct DescriptorLayoutHash
        {
            size_t operator()(const DescriptorLayoutBindings& info) const
            {
                return info.Hash();
            }
        };

        std::unordered_map<DescriptorLayoutBindings, vk::DescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
        vk::Device m_device {};
    };

    //--------------------------------------------//
    //---------  DescriptorLayoutBuilder  --------//
    //--------------------------------------------//
    class DescriptorLayoutBuilder
    {
	public:
        DescriptorLayoutBuilder() = delete;

        static DescriptorLayoutBuilder MakeDescriptorLayoutBuilder(DescriptorLayoutCache* layoutCache);

        DescriptorLayoutBuilder& SetBinding(uint32_t binding,
								            vk::DescriptorType type,
								            vk::ShaderStageFlags stageFlags);

        [[nodiscard]] DescriptorLayout BuildDescriptorLayout();

    private:
        template <uint32_t N_Sets>
        friend class DescriptorSetBuilder;

        DescriptorLayoutBuilder(DescriptorLayoutCache* layout_cache)
        : m_layoutCache(layout_cache)
        {}

        DescriptorLayoutBindings m_descriptor_layout_bindings {};

        DescriptorLayoutCache* m_layoutCache = nullptr;
    };


    //--------------------------------------------//
    //----------  DescriptorSetBuilder  ----------//
    //--------------------------------------------//
    template <uint32_t N_Sets>
    class DescriptorSetBuilder {
    public:
        DescriptorSetBuilder() = delete;

        static DescriptorSetBuilder MakeDescriptorSetBuilder(const DescriptorLayout& layout, DescriptorAllocator* descriptor_allocator);
        static DescriptorSetBuilder MakeDescriptorSetBuilder(DescriptorLayoutBuilder& layout_builder, DescriptorAllocator* descriptor_allocator);

        DescriptorSetBuilder& BindBuffer(uint32_t binding, std::array<vk::DescriptorBufferInfo, N_Sets> bufferInfo);

        DescriptorSetBuilder& BindImage (uint32_t binding, std::array<vk::DescriptorImageInfo,  N_Sets> imageInfo);

        
        bool BuildDescriptorSet(std::array<vk::DescriptorSet, N_Sets>& sets, vk::DescriptorSetLayout& layout);
        bool BuildDescriptorSet(std::array<vk::DescriptorSet, N_Sets>& sets);

    private:
        DescriptorSetBuilder(const DescriptorLayout& layout, DescriptorAllocator* descriptor_allocator)
        : m_descriptor_layout(layout), m_descriptorAlloc(descriptor_allocator)
        {}

        std::array<std::vector<vk::WriteDescriptorSet>, N_Sets> m_descriptorWrites;


        DescriptorLayout            m_descriptor_layout;
        DescriptorAllocator*        m_descriptorAlloc;
    };

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>
    DescriptorSetBuilder<N_Sets>::MakeDescriptorSetBuilder(
        const DescriptorLayout& layout,
        DescriptorAllocator* descriptor_allocator
    )
    {
        DescriptorSetBuilder set_builder {layout, descriptor_allocator};

        return set_builder;
    }

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets> DescriptorSetBuilder<N_Sets>::MakeDescriptorSetBuilder(
        DescriptorLayoutBuilder& layout_builder, 
        DescriptorAllocator* descriptor_allocator
    )
    {
        DescriptorSetBuilder set_builder { layout_builder.BuildDescriptorLayout(), descriptor_allocator };

        return set_builder;
    }

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>& DescriptorSetBuilder<N_Sets>::BindBuffer(
        uint32_t binding,
        std::array<vk::DescriptorBufferInfo, N_Sets> bufferInfo
    )
    {
        if (!m_descriptorAlloc)
        {
            BRR_LogError("Can't bind buffer with non-initialized DescriptorSetBuilder");
            return *this;
        }

        vk::DescriptorSetLayoutBinding descriptor_binding = m_descriptor_layout.m_bindings[binding];

        for (uint32_t info_idx = 0; info_idx < N_Sets; info_idx++)
        {
            vk::WriteDescriptorSet descriptor_write;
            descriptor_write
                .setPBufferInfo(&bufferInfo[info_idx])
                .setDstBinding(binding)
                .setDescriptorType(descriptor_binding.descriptorType)
                .setDescriptorCount(1);

            m_descriptorWrites[info_idx].push_back(descriptor_write);
        }

        return *this;
    }

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>& DescriptorSetBuilder<N_Sets>::BindImage(
        uint32_t binding,
        std::array<vk::DescriptorImageInfo, N_Sets> imageInfo
    )
    {
        if (!m_descriptorAlloc)
        {
            BRR_LogError("Can't bind image with non-initialized DescriptorSetBuilder");
            return *this;
        }

        vk::DescriptorSetLayoutBinding descriptor_binding = m_descriptor_layout.m_bindings[binding];

        for (uint32_t info_idx = 0; info_idx < N_Sets; info_idx++)
        {
            vk::WriteDescriptorSet descriptor_write;
            descriptor_write
                .setPImageInfo(imageInfo[info_idx])
                .setDstBinding(binding)
                .setDescriptorType(descriptor_binding.descriptorType)
                .setDescriptorCount(1);

            m_descriptorWrites[info_idx].push_back(descriptor_write);
        }

        return *this;
    }

    template <uint32_t N_Sets>
    bool DescriptorSetBuilder<N_Sets>::BuildDescriptorSet(std::array<vk::DescriptorSet, N_Sets>& sets,
                                                          vk::DescriptorSetLayout&               layout)
    {
        layout = m_descriptor_layout.m_descriptor_set_layout;
        std::vector<vk::DescriptorSetLayout> layouts (N_Sets, layout);

        // Allocate descriptor
        std::vector<vk::DescriptorSet> result_sets;
        bool success = m_descriptorAlloc->Allocate(layouts, result_sets);
        memcpy(sets.data(), result_sets.data(), N_Sets * sizeof(vk::DescriptorSet));
        if (!success) { return false; };

        for (uint32_t write_idx = 0; write_idx < N_Sets; write_idx++)
        {
            for (vk::WriteDescriptorSet& write : m_descriptorWrites[write_idx])
            {
                write.setDstSet(sets[write_idx]);
            }

            m_descriptorAlloc->GetDevice().updateDescriptorSets(m_descriptorWrites[write_idx], {});
        }

        return true;
    }

    template <uint32_t N_Sets>
    bool DescriptorSetBuilder<N_Sets>::BuildDescriptorSet(std::array<vk::DescriptorSet, N_Sets>& sets)
    {
        vk::DescriptorSetLayout layout;
        return BuildDescriptorSet(sets, layout);
    }
}

#endif