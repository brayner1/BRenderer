#ifndef BRR_NODECOMPONENT_H
#define BRR_NODECOMPONENT_H
#include "Scene/Entity.h"

namespace brr
{
	struct NodeComponent
	{
		static constexpr auto in_place_delete = true;

		[[nodiscard]] NodeComponent* GetParentNode() const { return mParent_; }
		[[nodiscard]] const std::vector<NodeComponent*>& GetChildren() const { return mChildren_; }

		Entity mEntity;

	private:
		friend struct Transform3DComponent;

		void SetParent(NodeComponent* parent);
		void RemoveChild(NodeComponent* child);

		NodeComponent* mParent_{ nullptr };
		std::vector<NodeComponent*> mChildren_{};
	};
}

#endif