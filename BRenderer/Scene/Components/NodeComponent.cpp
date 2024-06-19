#include "Scene/Components/NodeComponent.h"

namespace brr
{
	void NodeComponent::SetParent(NodeComponent* parent)
	{
		if (m_parent)
		{
			m_parent->RemoveChild(this);
		}

		m_parent = parent;
		if (m_parent)
		{
			m_parent->m_children.push_back(this);
		}

		GetEntity().GetScene()->ParentChanged(this);
	}

	void NodeComponent::RemoveChild(NodeComponent* child)
	{
		auto It = std::find(m_children.begin(), m_children.end(), child);
		assert(It != m_children.end() && "Children must be present to be removed!");

		m_children.erase(It);
		child->m_parent = nullptr;
	}
}
