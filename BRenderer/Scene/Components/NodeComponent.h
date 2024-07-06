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

		const std::string& GetName() const { return m_node_name; }
		void SetName(std::string node_name) { m_node_name = std::move(node_name); }

		[[nodiscard]] NodeComponent* GetParentNode() const { return m_parent; }
		[[nodiscard]] const std::vector<NodeComponent*>& GetChildren() const { return m_children; }

	private:
		friend struct Transform3DComponent;
		friend class Scene;

		void SetParent(NodeComponent* parent);
		void RemoveChild(NodeComponent* child);

		std::string m_node_name = "Object";

		NodeComponent* m_parent{ nullptr };
		std::vector<NodeComponent*> m_children{};
	};
}

#endif