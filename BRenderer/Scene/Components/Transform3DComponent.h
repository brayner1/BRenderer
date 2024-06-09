#ifndef BRR_TRANSFORM3DCOMPONENT_H
#define BRR_TRANSFORM3DCOMPONENT_H
#include <Scene/Components/EntityComponent.h>

#include <Renderer/SceneObjectsIDs.h>

namespace brr
{
    struct NodeComponent;

	struct alignas(64) Transform3DComponent : public EntityComponent
	{
		enum DirtyFlags
		{
			NOT_DIRTY = 0,
			GLOBAL_DIRTY = 1
		};

        explicit Transform3DComponent ();

		void SetTransformationMatrix(const glm::mat4& matrix);
		void SetTransform(const glm::vec3& position, const glm::fquat& rotation, const glm::vec3& scale);

        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::fquat& rotation);

        void Translate(const glm::vec3& translation);
        void Rotate(const glm::fquat& rotation);
        void Rotate(float angle,
                    const glm::vec3& axis);
        void SetScale(glm::vec3 scale);

        [[nodiscard]] const glm::vec3& GetLocalPosition() const { return m_position_; }
        [[nodiscard]] const glm::fquat& GetLocalRotation() const { return m_rotation_; }
        [[nodiscard]] const glm::vec3& GetLocalScale() const { return m_scale_; }
        [[nodiscard]] glm::vec3 GetLocalRotationEuler() const { return glm::eulerAngles(m_rotation_); }

        [[nodiscard]] glm::vec3 GetGlobalPosition() const;
        [[nodiscard]] glm::fquat GetGlobalRotation() const;
        [[nodiscard]] glm::vec3 GetGlobalScale() const;
        [[nodiscard]] glm::vec3 GetGlobalRotationEuler() const;

        [[nodiscard]] glm::mat4 GetMatrix() const;
        [[nodiscard]] const glm::mat4& GetGlobalMatrix() const;

        [[nodiscard]] DirtyFlags Dirty() const { return static_cast<DirtyFlags>(m_dirty_); }

        void SetParent(Transform3DComponent* parent);
        void RemoveChild(Transform3DComponent* child);

        [[nodiscard]] constexpr render::EntityID GetRenderEntityID() const { return m_render_entity_id; }

    public:

        void RegisterGraphics();
		void UnregisterGraphics();

    private:
        void PropagateTransformChange();

        glm::fquat m_rotation_{1.0, 0.0, 0.0, 0.0};
        glm::vec3 m_scale_{1.0f};
        glm::vec3 m_position_{0.0f};

        mutable uint8_t m_dirty_{GLOBAL_DIRTY};

        render::EntityID m_render_entity_id = render::EntityID::NULL_ID;

        alignas(16) mutable glm::mat4 m_global_transform_{1.f};
        // align matrix with 16 so it occupies exactly the second cache line.
    };
}

#endif
