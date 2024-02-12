#include "Scene/Scene.h"

#include <Scene/Components.h>
#include <Scene/Entity.h>
#include <Core/LogSystem.h>
#include <Visualization/SceneRenderer.h>


namespace brr
{
	Scene::Scene()
	{}

	Scene::Scene(PerspectiveCamera* camera) : m_camera(camera)
	{
	}

	Scene::~Scene()
	{
		m_registry.clear();
	}

    void Scene::InitSceneRenderer()
    {
		m_scene_renderer = std::make_unique<vis::SceneRenderer>(this);
    }

    Entity Scene::Add3DEntity(Entity parent)
	{
		BRR_LogInfo("Adding new 3D Entity");
		entt::entity new_entity = m_registry.create();

		Entity new_entity_struct(new_entity, this);

		NodeComponent& node = new_entity_struct.AddComponent<NodeComponent>();

		Transform3DComponent& transform = new_entity_struct.AddComponent<Transform3DComponent>();

		if (parent)
		{
			assert(m_registry.valid(parent.m_entity) && "Parent Entity must be valid.");
			transform.SetParent(&parent.GetComponent<Transform3DComponent>());
		}

		return Entity{new_entity, this};
	}

    void Scene::RemoveEntity(Entity entity)
    {
		if (!m_registry.valid(entity.m_entity))
		{
		    BRR_LogError("Trying to remove invalid entity. Entity Id '{}' is not valid.", uint32_t(entity.m_entity));
			return;
		}

		NodeComponent& node = entity.GetComponent<NodeComponent>();

        if (NodeComponent* parent_node = node.GetParentNode())
		{
		    parent_node->RemoveChild(&node);
		}

		for (NodeComponent* children : node.GetChildren())
		{
		    RemoveEntity(children->GetEntity());
		}

		m_registry.destroy(entity.m_entity);
    }
}
