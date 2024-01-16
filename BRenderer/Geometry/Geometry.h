#ifndef BRR_GEOMETRY_H
#define BRR_GEOMETRY_H
#include <Core/thirdpartiesInc.h>
#include <Renderer/Vulkan/VulkanInc.h>

namespace brr
{
	struct Vertex2_PosColor
	{
		glm::vec2 pos;
		glm::vec3 color;

		static vk::VertexInputBindingDescription GetBindingDescription();

		static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescriptions();
	};

	struct Vertex3_PosColor
	{
		glm::vec3 pos;
		glm::vec3 color;

		static vk::VertexInputBindingDescription GetBindingDescription();

		static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescriptions();
	};

	struct Vertex3_PosColorUV
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;

		static vk::VertexInputBindingDescription GetBindingDescription();

		static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescriptions();
	};

	struct Vertex3_PosUvNormal
	{
	    glm::vec3 pos;
		float u;
		glm::vec3 normal;
		float v;
		glm::vec3 tangent;

		static vk::VertexInputBindingDescription GetBindingDescription();

		static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescriptions();
	};

	using Vertex3 = Vertex3_PosUvNormal;
}

#endif