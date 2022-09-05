#include "Scene/Components.h"

namespace brr
{
	////////////////////////////
	/// Transform3DComponent ///
	///////////////////////////

	void Transform3DComponent::SetTransform(const glm::mat4& transform)
	{
		local_transform_ = transform;
		
		PropagateChildrenTransformChange();
	}

	void Transform3DComponent::SetPosition(const glm::vec3& position)
	{
		local_transform_[3] = glm::vec4{ position, 1.f };
	}

	void Transform3DComponent::SetRotation(const glm::fquat& rotation)
	{
		glm::mat4 result = glm::mat4_cast(rotation);
		result[3] = local_transform_[3];
		local_transform_ = result;
	}

	void Transform3DComponent::Translate(const glm::vec3& translation)
	{
		local_transform_ = glm::translate(local_transform_, translation);
	}

	void Transform3DComponent::Rotate(const glm::fquat& rotation)
	{
		local_transform_ = static_cast<glm::mat4>(rotation) * local_transform_;
	}

	void Transform3DComponent::Rotate(float angle, const glm::vec3& axis)
	{
		local_transform_ = glm::rotate(local_transform_, angle, axis);
	}

	void Transform3DComponent::Scale(glm::vec3 scale)
	{
		local_transform_ = glm::scale(local_transform_, scale);
	}

	glm::vec3 Transform3DComponent::GetPosition() const
	{
		return local_transform_[3];
	}

	glm::fquat Transform3DComponent::GetRotation() const
	{
		return glm::fquat{ local_transform_ };
	}

	glm::vec3 Transform3DComponent::GetEulerRotation() const
	{
		const glm::fquat rot{ local_transform_ };
		return glm::eulerAngles(rot);
	}

	const glm::mat4& Transform3DComponent::GetTransform() const
	{
		return  local_transform_;
	}

	const glm::mat4& Transform3DComponent::GetGlobalTransform()
	{
		if (dirty_ & GLOBAL_DIRTY)
		{
			global_transform_ = parent_? parent_->GetGlobalTransform() * local_transform_ : local_transform_;

			dirty_ &= ~GLOBAL_DIRTY;
		}
		return global_transform_;
	}

	void Transform3DComponent::SetParent(Transform3DComponent* parent)
	{
		if (parent_)
		{
			parent_->RemoveChild(this);
		}

		parent_ = parent;
		if (parent_)
		{
			parent_->children_.push_back(this);
		}

		PropagateChildrenTransformChange();
	}

	void Transform3DComponent::RemoveChild(Transform3DComponent* child)
	{
		auto It = std::find(children_.begin(), children_.end(), child);
		assert( It != children_.end() && "Children must be present to be removed!");

		children_.erase(It);
		child->parent_ = nullptr;
	}

	void Transform3DComponent::PropagateChildrenTransformChange()
	{
		dirty_ |= DirtyFlags::GLOBAL_DIRTY;

		for (Transform3DComponent* child : children_)
		{
			child->PropagateChildrenTransformChange();
		}
	}
}
