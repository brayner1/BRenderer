#ifndef BRR_GEOMETRY_H
#define BRR_GEOMETRY_H
#include <Core/thirdpartiesInc.h>

namespace brr
{
	struct Vertex2_PosColor
	{
		glm::vec2 pos;
		glm::vec3 color;
	};

	struct Vertex3_PosColor
	{
		glm::vec3 pos;
		glm::vec3 color;
	};

	struct Vertex3_PosColorUV
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;
	};

	struct Vertex3_PosUvNormal
	{
	    glm::vec3 pos;
		float u;
		glm::vec3 normal;
		float v;
		glm::vec3 tangent;
	};

	using Vertex3 = Vertex3_PosUvNormal;
}

#endif