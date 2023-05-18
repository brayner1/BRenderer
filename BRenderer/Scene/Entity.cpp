#include "Scene/Entity.h"

namespace brr
{

	Entity::Entity(entt::entity entity_handle, Scene* scene)
		: entity_(entity_handle), m_scene(scene)
	{
	}
}
