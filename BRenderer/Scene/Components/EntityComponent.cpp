#include <Scene/Components/EntityComponent.h>

#include <Scene/Components/NodeComponent.h>

namespace brr
{
    EntityComponent::EntityComponent(EntityComponent&& other) noexcept
    {
        *this = std::move(other);
    }

    EntityComponent& EntityComponent::operator=(EntityComponent&& other) noexcept
    {
        m_entity = other.m_entity;

        other.m_entity = {};
        return *this;
    }

    NodeComponent* EntityComponent::GetNodeComponent() const
    {
        return &m_entity.GetComponent<NodeComponent>();
    }
}
