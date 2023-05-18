#ifndef BRR_TRANSFORM3DCOMPONENT_H
#define BRR_TRANSFORM3DCOMPONENT_H
#include <Scene/Components/EntityComponent.h>

namespace brr
{
	struct NodeComponent;

	struct alignas(64) Transform3DComponent : public EntityComponent
	{
        explicit Transform3DComponent ();

		enum DirtyFlags
		{
			NOT_DIRTY = 0,
			GLOBAL_DIRTY = 1
		};

		void SetTransform(const glm::mat4& transform);
		void SetTransform(const glm::vec3& position, const glm::fquat& rotation, const glm::vec3& scale);

		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::fquat& rotation);

		void Translate(const glm::vec3& translation);
		void Rotate(const glm::fquat& rotation);
		void Rotate(float angle, const glm::vec3& axis);
		void SetScale(glm::vec3 scale);

		[[nodiscard]] const glm::vec3& GetLocalPosition() const { return m_position_; }
		[[nodiscard]] const glm::fquat& GetLocalRotation() const { return m_rotation_; }
		[[nodiscard]] const glm::vec3& GetLocalScale() const { return m_scale_; }
		[[nodiscard]] glm::vec3 GetLocalRotationEuler() const { return glm::eulerAngles(m_rotation_); }

		[[nodiscard]] glm::vec3 GetGlobalPosition();
		[[nodiscard]] glm::fquat GetGlobalRotation();
		[[nodiscard]] glm::vec3 GetGlobalScale();
		[[nodiscard]] glm::vec3 GetGlobalRotationEuler();

		[[nodiscard]] glm::mat4 GetTransform() const;
		[[nodiscard]] const glm::mat4& GetGlobalTransform();

		[[nodiscard]] DirtyFlags Dirty() const { return static_cast<DirtyFlags>(m_dirty_); }

		void SetParent(Transform3DComponent* parent);
		void RemoveChild(Transform3DComponent* child);

	private:
		void PropagateTransformChange();

		glm::fquat m_rotation_ {1.0, 0.0, 0.0, 0.0};
		glm::vec3 m_scale_	{1.0f};
		glm::vec3 m_position_ {0.0f};

		uint8_t m_dirty_{ GLOBAL_DIRTY };

		alignas(16) glm::mat4 m_global_transform_{ 1.f }; // align matrix with 16 so it occupies exactly the second cache line.
	};

}

#endif