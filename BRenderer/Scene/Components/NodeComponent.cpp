#include "Scene/Components/NodeComponent.h"

namespace brr
{
	void NodeComponent::SetParent(NodeComponent* parent)
	{
		if (mParent_)
		{
			mParent_->RemoveChild(this);
		}

		mParent_ = parent;
		if (mParent_)
		{
			mParent_->mChildren_.push_back(this);
		}
	}

	void NodeComponent::RemoveChild(NodeComponent* child)
	{
		auto It = std::find(mChildren_.begin(), mChildren_.end(), child);
		assert(It != mChildren_.end() && "Children must be present to be removed!");

		mChildren_.erase(It);
		child->mParent_ = nullptr;
	}
}
