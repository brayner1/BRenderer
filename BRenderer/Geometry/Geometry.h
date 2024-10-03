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

	struct Plane
	{
	    glm::vec3 point;
		glm::vec3 normal;
	};

	class AABBB
	{
	public:
		const glm::vec3& GetMinPos() const { return m_min_pos; }

		const glm::vec3& GetMaxPos() const { return m_max_pos; }

		bool IsValid() const
		{
		    return !glm::any(glm::lessThanEqual(m_max_pos, m_min_pos));
		}

		bool IsPointInside(const glm::vec3& point) const
        {
		    return glm::all(glm::greaterThanEqual(point, m_min_pos))
		        && glm::all(glm::lessThanEqual(point, m_max_pos));
		}

		bool IsInFrontOfPlane(const Plane& plane) const
		{
			if (!IsValid())
			{
			    return false;
			}

			const glm::vec3 extent = m_max_pos - m_min_pos;
			const glm::vec3 center = (m_max_pos + m_min_pos) / 2.0f;
		    const float r = extent.x * std::abs(plane.normal.x) + extent.y * std::abs(plane.normal.y) + extent.z * std::abs(plane.normal.z);
			const float dist = glm::dot(plane.normal, center - plane.point);

			return r <= dist;
		}

	private:

		glm::vec3 m_min_pos;
		glm::vec3 m_max_pos;
	};

	using Vertex3 = Vertex3_PosUvNormal;
}

#endif