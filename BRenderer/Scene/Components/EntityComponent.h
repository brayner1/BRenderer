#ifndef BRR_ENTITYCOMPONENT_H
#define BRR_ENTITYCOMPONENT_H
#include <Scene/Entity.h>

#include "Core/Events/Event.h"

namespace brr
{
    struct NodeComponent;
    class Scene;
    class Entity;

    class EntityComponent
    {
    public:

        EntityComponent () {}

        EntityComponent(EntityComponent&& other) noexcept;

        EntityComponent& operator=(EntityComponent&& other) noexcept;

        Scene* GetScene() const { return m_entity.GetScene(); }

        Entity GetEntity() const { return m_entity; }

        NodeComponent* GetNodeComponent() const;

    public:

        void OnInit() {}
        void OnDestroy() {}

        void RegisterGraphics() {}
        void UnregisterGraphics() {}

    private:
        friend class Entity;

        Entity m_entity;

    public:
        mutable Event<> m_destroyed_event;
    };
}

#endif