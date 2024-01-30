#ifndef BRR_ENTITYCOMPONENT_H
#define BRR_ENTITYCOMPONENT_H
#include <Scene/Entity.h>

namespace brr
{
    struct NodeComponent;
    class Scene;
    class Entity;

    class EntityComponent
    {
    public:

        EntityComponent () {}

        Scene* GetScene() const { return m_entity.GetScene(); }

        Entity GetEntity() const { return m_entity; }

        NodeComponent* GetNodeComponent() const;

    public:

        void OnInit() {}

    private:
        friend class Entity;

        void Init (Entity entity)
        {
            m_entity = entity;
        }

        Entity m_entity;
    };

}

#endif