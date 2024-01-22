#include "Descriptors.h"

#include "Vulkan/VulkanRenderDevice.h"



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

    //! MakeDescriptorLayoutBuilder
    DescriptorLayoutBuilder DescriptorLayoutBuilder::MakeDescriptorLayoutBuilder(VulkanRenderDevice* render_device)
    {
        DescriptorLayoutBuilder builder { render_device };

        return builder;
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
		descriptor_layout.m_layout_handle = m_render_device->CreateDescriptorSetLayout(m_descriptor_layout_bindings);
		descriptor_layout.m_bindings = std::move(m_descriptor_layout_bindings);
		return descriptor_layout;
	}
}
