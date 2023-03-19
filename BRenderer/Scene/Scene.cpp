#include "Scene.h"

#include "Components/NodeComponent.h"
#include "Renderer/Renderer.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"

namespace brr
{
	Scene::Scene()
	{}

	Scene::Scene(PerspectiveCamera* camera) : m_camera_(camera)
	{
		m_registry_.group<Transform3DComponent, Mesh3DComponent>();
	}

	Scene::~Scene()
	{
		m_registry_.clear();
	}

	Entity Scene::AddEntity(Entity parent)
	{
		entt::entity new_entity = m_registry_.create();

		NodeComponent& node = m_registry_.emplace<NodeComponent>(new_entity);
		Transform3DComponent& transform = m_registry_.emplace<Transform3DComponent>(new_entity);
		if (parent)
		{
			assert(m_registry_.valid(parent.entity_) && "Parent Entity must be valid.");
			transform.SetParent(&parent.GetComponent<Transform3DComponent>());
		}

		return Entity{new_entity, this};
	}

}
