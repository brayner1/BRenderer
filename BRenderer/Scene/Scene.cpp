#include "Scene.h"

#include "Renderer/Renderer.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"

namespace brr
{
	Scene::Scene()
	{
	}

	Scene::Scene(PerspectiveCamera* camera) : camera_(camera)
	{
		registry_.group<Transform3DComponent, Mesh3DComponent>();
	}

	Scene::~Scene()
	{
		registry_.clear();
	}

	Entity Scene::AddEntity(Entity parent)
	{
		entt::entity new_entity = registry_.create();

		Transform3DComponent& transform = registry_.emplace<Transform3DComponent>(new_entity);
		if (parent)
		{
			assert(registry_.valid(parent.entity_) && "Parent Entity must be valid.");
			transform.SetParent(&parent.GetComponent<Transform3DComponent>());
		}

		return Entity{new_entity, this};
	}

	void Scene::RenderScene()
	{
		auto group3dRender = registry_.group<Transform3DComponent, Mesh3DComponent>();

		render::Renderer* renderer = render::Renderer::GetRenderer();

		/*group3dRender.each([&](Entity entity, Transform3DComponent& transform, Mesh3DComponent& mesh)
		{
				
		});*/
	}
}
