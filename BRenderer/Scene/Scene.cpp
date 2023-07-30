#include "Scene/Scene.h"

#include <Scene/Components.h>
#include <Scene/Entity.h>
#include <Core/LogSystem.h>


namespace brr
{
	Scene::Scene()
	{}

	Scene::Scene(PerspectiveCamera* camera) : m_camera_(camera)
	{
	}

	Scene::~Scene()
	{
		m_registry_.clear();
	}

	Entity Scene::Add3DEntity(Entity parent)
	{
		BRR_LogInfo("Adding new 3D Entity");
		entt::entity new_entity = m_registry_.create();

		Entity new_entity_struct(new_entity, this);

		NodeComponent& node = new_entity_struct.AddComponent<NodeComponent>();

		Transform3DComponent& transform = new_entity_struct.AddComponent<Transform3DComponent>();

		if (parent)
		{
			assert(m_registry_.valid(parent.entity_) && "Parent Entity must be valid.");
			transform.SetParent(&parent.GetComponent<Transform3DComponent>());
		}

		return Entity{new_entity, this};
	}
}
