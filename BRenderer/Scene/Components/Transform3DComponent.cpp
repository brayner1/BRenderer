#include "Scene/Components/Transform3DComponent.h"

#include "Scene/Components/NodeComponent.h"

namespace brr
{
	////////////////////////////
	/// Transform3DComponent ///
	////////////////////////////

    Transform3DComponent::Transform3DComponent()
    {}

    void Transform3DComponent::SetTransform(const glm::mat4& transform)
	{
		//mLocal_transform_ = transform;
		glm::vec3 skew; glm::vec4 perspective;
		glm::decompose(transform, m_scale_, m_rotation_, m_position_, skew, perspective);

		PropagateTransformChange();
	}

	void Transform3DComponent::SetTransform(const glm::vec3& position, 
											const glm::fquat& rotation,
											const glm::vec3& scale)
	{
		m_position_ = position;
		m_rotation_ = rotation;
		m_scale_ = scale;
		PropagateTransformChange();
	}

	void Transform3DComponent::SetPosition(const glm::vec3& position)
	{
		//mLocal_transform_[3] = glm::vec4{ position, 1.f };
		m_position_ = position;
		PropagateTransformChange();
	}

	void Transform3DComponent::SetRotation(const glm::fquat& rotation)
	{
		/*glm::mat4 result = glm::mat4_cast(rotation);
		result[3] = mLocal_transform_[3];
		mLocal_transform_ = result;*/
		m_rotation_ = rotation;
		PropagateTransformChange();
	}

	void Transform3DComponent::Translate(const glm::vec3& translation)
	{
		//mLocal_transform_ = glm::translate(mLocal_transform_, translation);
		m_position_ += translation;
		PropagateTransformChange();
	}

	void Transform3DComponent::Rotate(const glm::fquat& rotation)
	{
		//mLocal_transform_ = static_cast<glm::mat4>(rotation) * mLocal_transform_;
		m_rotation_ = rotation * m_rotation_;
		PropagateTransformChange();
	}

	void Transform3DComponent::Rotate(float angle, const glm::vec3& axis)
	{
		//mLocal_transform_ = glm::rotate(mLocal_transform_, angle, axis);
		glm::fquat rotation(angle, axis);
		m_rotation_ = rotation * m_rotation_;
		PropagateTransformChange();
	}

	void Transform3DComponent::SetScale(glm::vec3 scale)
	{
		//mLocal_transform_ = glm::scale(mLocal_transform_, scale);
		m_scale_ = scale;
		PropagateTransformChange();
	}

	glm::vec3 Transform3DComponent::GetGlobalPosition()
	{
		const glm::mat4& global_transf = GetGlobalTransform();
		glm::vec3 position, scale, skew;
		glm::fquat rotation;
		glm::vec4 perspective;
		glm::decompose(global_transf, scale, rotation, position, skew, perspective);

		return position;
	}

	glm::fquat Transform3DComponent::GetGlobalRotation()
	{
		const glm::mat4& global_transf = GetGlobalTransform();
		glm::vec3 position, scale, skew;
		glm::fquat rotation;
		glm::vec4 perspective;
		glm::decompose(global_transf, scale, rotation, position, skew, perspective);

		return rotation;
	}

	glm::vec3 Transform3DComponent::GetGlobalScale()
	{
		const glm::mat4& global_transf = GetGlobalTransform();
		glm::vec3 position, scale, skew;
		glm::fquat rotation;
		glm::vec4 perspective;
		glm::decompose(global_transf, scale, rotation, position, skew, perspective);

		return scale;
	}

	glm::vec3 Transform3DComponent::GetGlobalRotationEuler()
	{
		const glm::mat4& global_transf = GetGlobalTransform();
		glm::vec3 position, scale, skew;
		glm::fquat rotation;
		glm::vec4 perspective;
		glm::decompose(global_transf, scale, rotation, position, skew, perspective);

		return glm::eulerAngles(rotation);
	}

	glm::mat4 Transform3DComponent::GetTransform() const
	{
		glm::mat4 local_transform = glm::mat4_cast(m_rotation_);
		glm::scale(local_transform, m_scale_);
		glm::translate(local_transform, m_position_);
		return  local_transform;
	}

	const glm::mat4& Transform3DComponent::GetGlobalTransform()
	{
		if (m_dirty_ & DirtyFlags::GLOBAL_DIRTY)
		{
			m_global_transform_ = GetTransform();
			if (NodeComponent* parent = GetNodeComponent()->GetParentNode())
			{
				Transform3DComponent& parent_transform = parent->GetEntity().GetComponent<Transform3DComponent>();
				m_global_transform_ = parent_transform.GetGlobalTransform() * m_global_transform_;
			}
			m_dirty_ &= ~DirtyFlags::GLOBAL_DIRTY;
		}
		return m_global_transform_;
	}

	void Transform3DComponent::SetParent(Transform3DComponent* parent)
	{
		GetNodeComponent()->SetParent(parent->GetNodeComponent());
		PropagateTransformChange();
	}

	void Transform3DComponent::RemoveChild(Transform3DComponent* child)
	{
		assert((child != nullptr) && "You can't remove null child.");
		GetNodeComponent()->RemoveChild(child->GetNodeComponent());
		child->PropagateTransformChange();
	}

	void Transform3DComponent::PropagateTransformChange()
	{
		m_dirty_ |= DirtyFlags::GLOBAL_DIRTY | DirtyFlags::RENDER_DIRTY;

		for (NodeComponent* child : GetNodeComponent()->mChildren_)
		{
			Transform3DComponent& child_transform = child->GetEntity().GetComponent<Transform3DComponent>();
			child_transform.PropagateTransformChange();
		}
	}
}