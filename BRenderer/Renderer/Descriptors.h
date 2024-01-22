#ifndef BRR_DESCRIPTORS_H
#define BRR_DESCRIPTORS_H
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/RenderEnums.h>
#include <Core/LogSystem.h>

namespace brr::render
{
    class VulkanRenderDevice;

    //--------------------------------------------//
    //--------  DescriptorLayoutBindings  --------//
    //--------------------------------------------//
    struct DescriptorLayoutBinding
    {
        uint32_t binding;
        DescriptorType descriptor_type;
        ShaderStageFlag shader_stage_flag;
    };

    struct DescriptorLayoutBindings
    {
        DescriptorLayoutBindings() = default;

        bool operator==(const DescriptorLayoutBindings& other) const;

        DescriptorLayoutBinding& operator [] (uint32_t binding) { return m_bindings[binding]; }

        std::vector<DescriptorLayoutBinding> m_bindings {};
    };

    //--------------------------------------------//
    //------------  DescriptorLayout  ------------//
    //--------------------------------------------//
    struct DescriptorLayout
    {
        DescriptorLayoutHandle m_layout_handle;
        DescriptorLayoutBindings m_bindings;
    };

    //--------------------------------------------//
    //---------  DescriptorLayoutBuilder  --------//
    //--------------------------------------------//
    class DescriptorLayoutBuilder
    {
	public:
        DescriptorLayoutBuilder() = delete;

        static DescriptorLayoutBuilder MakeDescriptorLayoutBuilder(VulkanRenderDevice* render_device);

        DescriptorLayoutBuilder& SetBinding(uint32_t binding,
								            DescriptorType type,
								            ShaderStageFlag stageFlags);

        [[nodiscard]] DescriptorLayout BuildDescriptorLayout();

    private:
        template <uint32_t N_Sets>
        friend class DescriptorSetBuilder;

        DescriptorLayoutBuilder(VulkanRenderDevice* render_device)
        : m_render_device(render_device)
        {}

        DescriptorLayoutBindings m_descriptor_layout_bindings {};

        VulkanRenderDevice* m_render_device = nullptr;
    };


    //--------------------------------------------//
    //----------  DescriptorSetBuilder  ----------//
    //--------------------------------------------//
    template <uint32_t N_Sets>
    class DescriptorSetBuilder {
    public:
        DescriptorSetBuilder() = delete;

        static DescriptorSetBuilder MakeDescriptorSetBuilder(const DescriptorLayout& layout, VulkanRenderDevice* render_device);
        static DescriptorSetBuilder MakeDescriptorSetBuilder(DescriptorLayoutBuilder& layout_builder, VulkanRenderDevice* render_device);

        DescriptorSetBuilder& BindBuffer(uint32_t binding, std::array<BufferHandle, N_Sets> buffers_info);

        DescriptorSetBuilder& BindImage (uint32_t binding, std::array<Texture2DHandle,  N_Sets> images_info);

        
        bool BuildDescriptorSet(std::array<DescriptorSetHandle, N_Sets>& sets, DescriptorLayoutHandle& layout);
        bool BuildDescriptorSet(std::array<DescriptorSetHandle, N_Sets>& sets);

    private:
        DescriptorSetBuilder(const DescriptorLayout& layout, VulkanRenderDevice* render_device)
        : m_descriptor_layout(layout), m_render_device(render_device)
        {}

        struct ShaderSetBinding
        {
            BufferHandle buffer_handle = null_handle;
            Texture2DHandle texture_handle = null_handle;
        };

        std::array<std::vector<ShaderSetBinding>, N_Sets> m_descriptor_writes;

        DescriptorLayout            m_descriptor_layout;
        VulkanRenderDevice*         m_render_device;
    };

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>
    DescriptorSetBuilder<N_Sets>::MakeDescriptorSetBuilder(
        const DescriptorLayout& layout,
        VulkanRenderDevice* render_device
    )
    {
        DescriptorSetBuilder set_builder {layout, render_device};

        return set_builder;
    }

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets> DescriptorSetBuilder<N_Sets>::MakeDescriptorSetBuilder(
        DescriptorLayoutBuilder& layout_builder, 
        VulkanRenderDevice* render_device
    )
    {
        DescriptorSetBuilder set_builder { layout_builder.BuildDescriptorLayout(), render_device };

        return set_builder;
    }

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>& DescriptorSetBuilder<N_Sets>::BindBuffer(
        uint32_t binding,
        std::array<BufferHandle, N_Sets> bufferInfo
    )
    {
        if (!m_render_device)
        {
            BRR_LogError("Can't bind buffer with non-initialized DescriptorSetBuilder");
            return *this;
        }

        //DescriptorLayoutBinding descriptor_binding = m_descriptor_layout.m_bindings[binding];
        //vk::DescriptorType vk_descriptor_type = VkHelpers::VkDescriptorTypeFromDescriptorType(descriptor_binding.descriptor_type);
        

        for (uint32_t info_idx = 0; info_idx < N_Sets; info_idx++)
        {
            /*vk::WriteDescriptorSet descriptor_write;
            descriptor_write
                .setPBufferInfo(&bufferInfo[info_idx])
                .setDstBinding(binding)
                .setDescriptorType(vk_descriptor_type)
                .setDescriptorCount(1);*/

            ShaderSetBinding descriptor_write;
            descriptor_write.buffer_handle = bufferInfo[info_idx];
            m_descriptor_writes[info_idx].push_back(descriptor_write);
        }

        return *this;
    }

    template <uint32_t N_Sets>
    DescriptorSetBuilder<N_Sets>& DescriptorSetBuilder<N_Sets>::BindImage(
        uint32_t binding,
        std::array<Texture2DHandle, N_Sets> imageInfo
    )
    {
        if (!m_render_device)
        {
            BRR_LogError("Can't bind image with non-initialized DescriptorSetBuilder");
            return *this;
        }

        //DescriptorLayoutBinding descriptor_binding = m_descriptor_layout.m_bindings[binding];
        //vk::DescriptorType vk_descriptor_type = VkHelpers::VkDescriptorTypeFromDescriptorType(descriptor_binding.descriptor_type);

        for (uint32_t info_idx = 0; info_idx < N_Sets; info_idx++)
        {
            /*vk::WriteDescriptorSet descriptor_write;
            descriptor_write
                .setPImageInfo(&imageInfo[info_idx])
                .setDstBinding(binding)
                .setDescriptorType(vk_descriptor_type)
                .setDescriptorCount(1);*/

            ShaderSetBinding descriptor_write;
            descriptor_write.texture_handle = imageInfo[info_idx];
            m_descriptor_writes[info_idx].push_back(descriptor_write);
        }

        return *this;
    }

    template <uint32_t N_Sets>
    bool DescriptorSetBuilder<N_Sets>::BuildDescriptorSet(std::array<DescriptorSetHandle, N_Sets>& sets,
                                                          DescriptorLayoutHandle&               layout)
    {
        layout = m_descriptor_layout.m_layout_handle;

        std::array<std::vector<VKRD::DescriptorSetBinding>, N_Sets> descriptor_set_bindings;
        for (uint32_t desc_idx = 0; desc_idx < N_Sets; desc_idx++)
        {
            auto& desc_write_vec = descriptor_set_bindings[desc_idx];
            for (uint32_t binding_idx = 0; binding_idx < m_descriptor_writes[desc_idx].size(); binding_idx++)
            {
                VKRD::DescriptorSetBinding binding;
                binding.descriptor_type = m_descriptor_layout.m_bindings[binding_idx].descriptor_type;
                if (m_descriptor_writes[desc_idx][binding_idx].buffer_handle != null_handle)
                {
                    binding.buffer_handle = m_descriptor_writes[desc_idx][binding_idx].buffer_handle;
                }
                if (m_descriptor_writes[desc_idx][binding_idx].texture_handle != null_handle)
                {
                    binding.texture_handle = m_descriptor_writes[desc_idx][binding_idx].texture_handle;
                }
                desc_write_vec.push_back(binding);
            }
        }

        // Allocate descriptor
        std::vector<DescriptorSetHandle> result_sets = m_render_device->AllocateDescriptorSet(layout, N_Sets, descriptor_set_bindings);
        bool                           success     = !result_sets.empty();
        if (!success) { return false; }
        memcpy(sets.data(), result_sets.data(), N_Sets * sizeof(DescriptorSetHandle));

        return true;
    }

    template <uint32_t N_Sets>
    bool DescriptorSetBuilder<N_Sets>::BuildDescriptorSet(std::array<DescriptorSetHandle, N_Sets>& sets)
    {
        DescriptorLayoutHandle layout;
        return BuildDescriptorSet(sets, layout);
    }
}

#endif