#ifndef BRR_DESCRIPTORS_H
#define BRR_DESCRIPTORS_H
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/RenderEnums.h>
#include <Core/LogSystem.h>

namespace brr::render
{
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
        DescriptorLayoutBuilder();

        DescriptorLayoutBuilder& SetBinding(uint32_t binding,
								            DescriptorType type,
								            ShaderStageFlag stageFlags);

        [[nodiscard]] DescriptorLayout BuildDescriptorLayout();

    private:
        friend class DescriptorSetUpdater;

        DescriptorLayoutBuilder(VulkanRenderDevice* render_device)
        : m_render_device(render_device)
        {}

        DescriptorLayoutBindings m_descriptor_layout_bindings {};

        VulkanRenderDevice* m_render_device = nullptr;
    };

    struct DescriptorSetBinding
    {
        uint32_t        descriptor_binding;
        DescriptorType  descriptor_type;
        Texture2DHandle texture_handle = null_handle;
        BufferHandle    buffer_handle = null_handle;
        uint32_t        buffer_size;
        uint32_t        buffer_offset;
    };

    //--------------------------------------------//
    //----------  DescriptorSetUpdater  ----------//
    //--------------------------------------------//

    // Helper class for updating descriptor set bindings.
    class DescriptorSetUpdater {
    public:
        DescriptorSetUpdater() = delete;

        DescriptorSetUpdater(const DescriptorLayout& layout)
        : m_descriptor_layout(layout),
          m_render_device(VKRD::GetSingleton())
        {}

        DescriptorSetUpdater& BindBuffer(uint32_t binding, const BufferHandle& buffers_info, uint32_t buffer_size);

        DescriptorSetUpdater& BindImage (uint32_t binding, const Texture2DHandle& images_info);


        bool UpdateDescriptorSet(const DescriptorSetHandle& sets);

    private:

        std::vector<DescriptorSetBinding> m_descriptor_writes;

        DescriptorLayout            m_descriptor_layout;
        VulkanRenderDevice*         m_render_device;
    };
}

#endif