#ifndef BRR_NODECOMPONENT_H
#define BRR_NODECOMPONENT_H
#include <Scene/Components/EntityComponent.h>

namespace brr
{
	struct NodeComponent : public EntityComponent
	{
		static constexpr auto in_place_delete = true;

		NodeComponent ()
		{}

		[[nodiscard]] NodeComponent* GetParentNode() const { return m_parent; }
		[[nodiscard]] const std::vector<NodeComponent*>& GetChildren() const { return m_children; }

	private:
		friend struct Transform3DComponent;
		friend class Scene;

		void SetParent(NodeComponent* parent);
		void RemoveChild(NodeComponent* child);

		NodeComponent* m_parent{ nullptr };
		std::vector<NodeComponent*> m_children{};
	};
}

#endif