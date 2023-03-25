#ifndef BRR_DESCRIPTORS_H
#define BRR_DESCRIPTORS_H

namespace brr::render
{
    //-----------------
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

    //-----------------
    class DescriptorLayoutCache
    {
    public:
        DescriptorLayoutCache() = default;
        explicit DescriptorLayoutCache(vk::Device device);

        vk::DescriptorSetLayout CreateDescriptorLayout(vk::DescriptorSetLayoutCreateInfo* layout_create_info);

        void Cleanup();

        [[nodiscard]] bool IsValid() const { return m_device; }

        struct DescriptorLayoutInfo
        {
            std::vector<vk::DescriptorSetLayoutBinding> m_bindings;

            bool operator==(const DescriptorLayoutInfo& other) const;

            size_t Hash() const;
        };

    private:

        struct DescriptorLayoutHash
        {
            size_t operator()(const DescriptorLayoutInfo& info) const
            {
                return info.Hash();
            }
        };

        std::unordered_map<DescriptorLayoutInfo, vk::DescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
        vk::Device m_device {};
    };

    //---------------
    class DescriptorLayoutBuilder
    {
	public:
        static DescriptorLayoutBuilder MakeDescriptorLayoutBuilder(DescriptorLayoutCache* layoutCache);

        DescriptorLayoutBuilder& SetBinding(uint32_t binding,
								            vk::DescriptorType type,
								            vk::ShaderStageFlags stageFlags);

        const std::vector<vk::DescriptorSetLayoutBinding>& GetBindings() const { return m_layoutBindings; }

        [[nodiscard]] vk::DescriptorSetLayout GetDescriptorLayout() const;
	private:
        DescriptorLayoutBuilder() = default;

        std::vector<vk::DescriptorSetLayoutBinding> m_layoutBindings;

        DescriptorLayoutCache* m_layoutCache = nullptr;
    };

    template <uint32_t N_Sets>
    class DescriptorSetBuilder {
    public:

        static DescriptorSetBuilder MakeDescriptorSetBuilder(DescriptorLayoutBuilder* layoutBuilder, DescriptorAllocator* descriptorAllocator);

        DescriptorSetBuilder& BindBuffer(uint32_t binding, std::array<vk::DescriptorBufferInfo, N_Sets> bufferInfo);

        DescriptorSetBuilder& BindImage (uint32_t binding, std::array<vk::DescriptorImageInfo,  N_Sets> imageInfo);

        
        bool BuildDescriptorSet(std::array<vk::DescriptorSet, N_Sets>& sets, vk::DescriptorSetLayout& layout);
        bool BuildDescriptorSet(std::array<vk::DescriptorSet, N_Sets>& sets);
    private:
        DescriptorSetBuilder() = default;

        std::array<std::vector<vk::WriteDescriptorSet>, N_Sets> m_descriptorWrites;


        DescriptorLayoutBuilder*	m_descriptorLayoutBuilder = nullptr;
        DescriptorAllocator*        m_descriptorAlloc = nullptr;
    };

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>
    DescriptorSetBuilder<N_Sets>::MakeDescriptorSetBuilder(
        DescriptorLayoutBuilder* layoutBuilder, 
        DescriptorAllocator* descriptorAllocator
    )
    {
        DescriptorSetBuilder builder;

        builder.m_descriptorLayoutBuilder = layoutBuilder;
        builder.m_descriptorAlloc = descriptorAllocator;
        return builder;
    }

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>& DescriptorSetBuilder<N_Sets>::BindBuffer(uint32_t binding,
        std::array<vk::DescriptorBufferInfo, N_Sets> bufferInfo)
    {
        if (!m_descriptorAlloc || !m_descriptorLayoutBuilder)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Can't bind buffer with non-initialized DescriptorSetBuilder");
            return *this;
        }

        vk::DescriptorSetLayoutBinding setLayoutBinding = m_descriptorLayoutBuilder->GetBindings()[binding];

        for (uint32_t info_idx = 0; info_idx < N_Sets; info_idx++)
        {
            vk::WriteDescriptorSet descriptor_write;
            descriptor_write
                .setPBufferInfo(&bufferInfo[info_idx])
                .setDstBinding(binding)
                .setDescriptorType(setLayoutBinding.descriptorType)
                .setDescriptorCount(1);

            m_descriptorWrites[info_idx].push_back(descriptor_write);
        }

        return *this;
    }

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>& DescriptorSetBuilder<N_Sets>::BindImage(uint32_t binding,
        std::array<vk::DescriptorImageInfo, N_Sets> imageInfo)
    {
        if (!m_descriptorAlloc || !m_descriptorLayoutBuilder)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Can't bind image with non-initialized DescriptorSetBuilder");
            return *this;
        }

        vk::DescriptorSetLayoutBinding setLayoutBinding = m_descriptorLayoutBuilder->GetBindings()[binding];

        for (uint32_t info_idx = 0; info_idx < N_Sets; info_idx++)
        {
            vk::WriteDescriptorSet descriptor_write;
            descriptor_write
                .setPImageInfo(imageInfo[info_idx])
                .setDstBinding(binding)
                .setDescriptorType(setLayoutBinding.descriptorType)
                .setDescriptorCount(1);

            m_descriptorWrites[info_idx].push_back(descriptor_write);
        }

        return *this;
    }

    template <uint32_t N_Sets>
    bool DescriptorSetBuilder<N_Sets>::BuildDescriptorSet(std::array<vk::DescriptorSet, N_Sets>& sets,
        vk::DescriptorSetLayout& layout)
    {
        layout = m_descriptorLayoutBuilder->GetDescriptorLayout();
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