#include "Scene.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"

namespace brr
{
	Entity Scene::AddEntity()
	{
		entt::entity new_entity = registry_.create();


		registry_.emplace<Transform3DComponent>(new_entity);

		return Entity{new_entity, this};
	}
}
