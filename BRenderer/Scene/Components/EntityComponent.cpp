#include <Scene/Components/EntityComponent.h>

#include <Scene/Components/NodeComponent.h>

namespace brr
{
    NodeComponent* EntityComponent::GetNodeComponent() const
    {
        return &m_entity.GetComponent<NodeComponent>();
    }
}
