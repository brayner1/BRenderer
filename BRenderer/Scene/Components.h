#ifndef BRR_COMPONENTS_H
#define BRR_COMPONENTS_H
#include "Components.h"
#include "Entity.h"
//#include "glm/glm.hpp"

namespace brr
{
	
	struct alignas(64) Transform3DComponent
	{
		static constexpr auto in_place_delete = true;

		enum DirtyFlags
		{
			NOT_DIRTY = 0,
			GLOBAL_DIRTY = 1
		};

		void SetTransform(const glm::mat4& transform);
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::fquat& rotation);

		void Translate(const glm::vec3& translation);
		void Rotate(const glm::fquat& rotation);
		void Rotate(float angle, const glm::vec3& axis);
		void Scale(glm::vec3 scale);

		[[nodiscard]] glm::vec3 GetPosition() const;
		[[nodiscard]] glm::fquat GetRotation() const;
		[[nodiscard]] glm::vec3 GetEulerRotation() const;

		[[nodiscard]] const glm::mat4& GetTransform() const;
		[[nodiscard]] const glm::mat4& GetGlobalTransform();

		void SetParent(Transform3DComponent* parent);

		void RemoveChild(Transform3DComponent* child);
		

	private:
		void PropagateChildrenTransformChange();

		glm::mat4 local_transform_ { 1.f };
		glm::mat4 global_transform_{ 1.f };

		Transform3DComponent* parent_ { nullptr };
		std::vector<Transform3DComponent*> children_ {};

		uint8_t dirty_{ NOT_DIRTY };
	};

	struct Mesh3DComponent
	{
		std::vector<glm::vec3> vertices {};
		std::vector<uint32_t> indices {};
	};

}

#endif