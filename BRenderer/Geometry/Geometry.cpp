#include "Geometry.h"

namespace brr
{
	vk::VertexInputBindingDescription Vertex2_PosColor::GetBindingDescription()
	{
		vk::VertexInputBindingDescription binding_description {};

		binding_description
			.setBinding(0)
			.setStride(sizeof(Vertex2_PosColor))
			.setInputRate(vk::VertexInputRate::eVertex);

		return binding_description;
	}

	std::array<vk::VertexInputAttributeDescription, 2> Vertex2_PosColor::GetAttributeDescriptions()
	{
		std::array<vk::VertexInputAttributeDescription, 2> attribute_descriptions {};

		// Position AttributeDescription
		{
			attribute_descriptions[0]
				.setBinding(0)
				.setLocation(0)
				.setFormat(vk::Format::eR32G32Sfloat)
				.setOffset(offsetof(Vertex2_PosColor, pos));
		}

		// Color AttributeDescription
		{
			attribute_descriptions[1]
				.setBinding(0)
				.setLocation(1)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(Vertex2_PosColor, color));
		}

		return attribute_descriptions;
	}

	vk::VertexInputBindingDescription Vertex3_PosColor::GetBindingDescription()
	{
		vk::VertexInputBindingDescription binding_description{};

		binding_description
			.setBinding(0)
			.setStride(sizeof(Vertex3_PosColor))
			.setInputRate(vk::VertexInputRate::eVertex);

		return binding_description;
	}

	std::array<vk::VertexInputAttributeDescription, 2> Vertex3_PosColor::GetAttributeDescriptions()
	{
		std::array<vk::VertexInputAttributeDescription, 2> attribute_descriptions{};

		// Position AttributeDescription
		{
			attribute_descriptions[0]
				.setBinding(0)
				.setLocation(0)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(Vertex3_PosColor, pos));
		}

		// Color AttributeDescription
		{
			attribute_descriptions[1]
				.setBinding(0)
				.setLocation(1)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(Vertex3_PosColor, color));
		}

		return attribute_descriptions;
	}

    vk::VertexInputBindingDescription Vertex3_PosColorUV::GetBindingDescription()
    {
		vk::VertexInputBindingDescription binding_description{};

		binding_description
			.setBinding(0)
			.setStride(sizeof(Vertex3_PosColorUV))
			.setInputRate(vk::VertexInputRate::eVertex);

		return binding_description;
    }

    std::array<vk::VertexInputAttributeDescription, 3> Vertex3_PosColorUV::GetAttributeDescriptions()
    {
		std::array<vk::VertexInputAttributeDescription, 3> attribute_descriptions{};

		// Position AttributeDescription
		{
			attribute_descriptions[0]
				.setBinding(0)
				.setLocation(0)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(Vertex3_PosColorUV, pos));
		}

		// Color AttributeDescription
		{
			attribute_descriptions[1]
				.setBinding(0)
				.setLocation(1)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(Vertex3_PosColorUV, color));
		}

		// Color AttributeDescription
		{
			attribute_descriptions[2]
				.setBinding(0)
				.setLocation(2)
				.setFormat(vk::Format::eR32G32Sfloat)
				.setOffset(offsetof(Vertex3_PosColorUV, uv));
		}

		return attribute_descriptions;
    }

    vk::VertexInputBindingDescription Vertex3_PosUvNormal::GetBindingDescription()
    {
		vk::VertexInputBindingDescription binding_description{};

		binding_description
			.setBinding(0)
			.setStride(sizeof(Vertex3_PosUvNormal))
			.setInputRate(vk::VertexInputRate::eVertex);

		return binding_description;
    }

    std::array<vk::VertexInputAttributeDescription, 5> Vertex3_PosUvNormal::GetAttributeDescriptions()
    {
		std::array<vk::VertexInputAttributeDescription, 5> attribute_descriptions{};

		// Position AttributeDescription
		{
			attribute_descriptions[0]
				.setBinding(0)
				.setLocation(0)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(Vertex3_PosUvNormal, pos));
		}

		// U AttributeDescription
		{
			attribute_descriptions[1]
				.setBinding(0)
				.setLocation(1)
				.setFormat(vk::Format::eR32Sfloat)
				.setOffset(offsetof(Vertex3_PosUvNormal, u));
		}

		// Normal AttributeDescription
		{
			attribute_descriptions[2]
				.setBinding(0)
				.setLocation(2)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(Vertex3_PosUvNormal, normal));
		}

		// Normal AttributeDescription
		{
			attribute_descriptions[3]
				.setBinding(0)
				.setLocation(3)
				.setFormat(vk::Format::eR32Sfloat)
				.setOffset(offsetof(Vertex3_PosUvNormal, v));
		}

		// Tangent AttributeDescription
		{
			attribute_descriptions[4]
				.setBinding(0)
				.setLocation(4)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(Vertex3_PosUvNormal, tangent));
		}

		return attribute_descriptions;
    }
}
