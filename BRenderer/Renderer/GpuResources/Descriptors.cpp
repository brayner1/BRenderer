#include "Descriptors.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>



namespace brr::render
{
    //--------------------------------------------//
    //--------  DescriptorLayoutBindings  --------//
    //--------------------------------------------//

    bool DescriptorLayoutBindings::operator==(const DescriptorLayoutBindings& other) const
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
                if (other.m_bindings[i].descriptor_type != m_bindings[i].descriptor_type)
                {
                    return false;
                }
                //if (other.m_bindings[i].descriptorCount != m_bindings[i].descriptorCount)
                //{
                //	return false;
                //}
                if (other.m_bindings[i].shader_stage_flag != m_bindings[i].shader_stage_flag)
                {
                    return false;
                }
            }
            return true;
        }
    }

    //--------------------------------------------//
    //--------  DescriptorLayoutBuilder  ---------//
    //--------------------------------------------//

    DescriptorLayoutBuilder::DescriptorLayoutBuilder()
    : m_render_device (VKRD::GetSingleton())
    {
        //
    }

    //! SetBinding
    DescriptorLayoutBuilder& DescriptorLayoutBuilder::SetBinding(uint32_t binding, 
                                                                 DescriptorType type,
                                                                 ShaderStageFlag stageFlags)
    {
        DescriptorLayoutBinding layout_binding {binding, type, stageFlags};

        m_descriptor_layout_bindings.m_bindings.push_back(layout_binding);

        return *this;
    }

    DescriptorLayout DescriptorLayoutBuilder::BuildDescriptorLayout()
    {
        DescriptorLayout descriptor_layout;
        descriptor_layout.m_layout_handle = m_render_device->DescriptorSetLayout_Create(m_descriptor_layout_bindings);
        descriptor_layout.m_bindings = std::move(m_descriptor_layout_bindings);
        return descriptor_layout;
    }

    //--------------------------------------------//
    //----------  DescriptorSetUpdater  ----------//
    //--------------------------------------------//

    DescriptorSetUpdater& DescriptorSetUpdater::BindBuffer(uint32_t            binding,
                                                           const BufferHandle& bufferInfo,
                                                           uint32_t            buffer_size,
                                                           uint32_t            buffer_offset)
    {
        if (!m_render_device)
        {
            BRR_LogError("Can't bind buffer with non-initialized DescriptorSetUpdater");
            return *this;
        }

        if (binding >= m_descriptor_layout.m_bindings.m_bindings.size())
        {
            BRR_LogError(
                "Binding buffer on invalid descriptor set binding index.\nPassed binding: {}, Layout max bindings: {}",
                binding, m_descriptor_layout.m_bindings.m_bindings.size());
            return *this;
        }

        DescriptorSetBinding descriptor_write;
        descriptor_write.buffer_handle = bufferInfo;
        descriptor_write.descriptor_binding = binding;
        descriptor_write.descriptor_type = m_descriptor_layout.m_bindings[binding].descriptor_type;
        descriptor_write.buffer_size = buffer_size;
        descriptor_write.buffer_offset = buffer_offset;
        m_descriptor_writes.push_back(descriptor_write);

        return *this;
    }

    DescriptorSetUpdater& DescriptorSetUpdater::BindImage(uint32_t binding,
                                                          const Texture2DHandle& imageInfo)
    {
        if (!m_render_device)
        {
            BRR_LogError("Can't bind image with non-initialized DescriptorSetUpdater");
            return *this;
        }

        if (binding >= m_descriptor_layout.m_bindings.m_bindings.size())
        {
            BRR_LogError("Binding image on invalid descriptor set binding index.");
            return *this;
        }


        DescriptorSetBinding descriptor_write;
        descriptor_write.texture_handle = imageInfo;
        descriptor_write.descriptor_binding = binding;
        descriptor_write.descriptor_type = m_descriptor_layout.m_bindings[binding].descriptor_type;
        m_descriptor_writes.push_back(descriptor_write);

        return *this;
    }

    bool DescriptorSetUpdater::UpdateDescriptorSet(const DescriptorSetHandle& sets)
    {
        bool success = true;
        // Allocate descriptor
        success = success && m_render_device->DescriptorSet_UpdateResources(sets, m_descriptor_writes);

        return success;
    }
}
