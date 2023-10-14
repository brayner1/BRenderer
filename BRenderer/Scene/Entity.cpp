#include "Scene/Entity.h"

namespace brr
{

	Entity::Entity(entt::entity entity_handle, Scene* scene)
		: m_scene(scene), m_entity(entity_handle)
    {
	}
}
