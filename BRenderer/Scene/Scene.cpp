#include "Scene/Scene.h"

#include "Scene/Components.h"
#include "Scene/Entity.h"
#include "Scene/Components/NodeComponent.h"
#include "Core/LogSystem.h"


namespace brr
{
	Scene::Scene() : m_scene_renderer(m_registry_)
	{}

	Scene::Scene(PerspectiveCamera* camera) : m_scene_renderer(m_registry_), m_camera_(camera)
	{
		m_registry_.group<Transform3DComponent, Mesh3DComponent>();
	}

	Scene::~Scene()
	{
		m_registry_.clear();
	}

	Entity Scene::AddEntity(Entity parent)
	{
		BRR_LogInfo("Adding new Entity");
		entt::entity new_entity = m_registry_.create();

		NodeComponent& node = m_registry_.emplace<NodeComponent>(new_entity);
		Transform3DComponent& transform = m_registry_.emplace<Transform3DComponent>(new_entity, &node);
		if (parent)
		{
			assert(m_registry_.valid(parent.entity_) && "Parent Entity must be valid.");
			transform.SetParent(&parent.GetComponent<Transform3DComponent>());
		}

		return Entity{new_entity, this};
	}

}
