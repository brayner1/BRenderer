#ifndef BRR_ENTITY_H
#define BRR_ENTITY_H
#include <Scene/Scene.h>

namespace brr
{
    class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity entity_handle, Scene* scene);

		template <typename T, std::enable_if_t<!std::is_empty_v<T>, bool> = true, typename... Args>
		T& AddComponent(Args&&... args);

		template <typename T, std::enable_if_t<std::is_empty_v<T>, bool> = true>
		void AddComponent();

		template<typename T>
		T& GetComponent() const;

		template<typename T>
		void RemoveComponent();

		template<typename T>
		[[nodiscard]] bool HasComponent() const;

		template<typename... T>
		[[nodiscard]] bool HasAllComponents() const;

		bool IsValid() const { return m_entity != entt::null && m_scene != nullptr; }
		operator bool() const { return IsValid(); }

		Scene* GetScene() const { return m_scene; }

	private:
		friend class Scene;

		Scene* m_scene = nullptr;
		entt::entity m_entity{ entt::null };
	};

	/**********************
	 *** Implementation ***
	 *********************/

	template <typename T, std::enable_if_t<!std::is_empty_v<T>, bool>, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		assert(IsValid() && "Entity must be valid to add a component.");
		assert(!m_scene->m_registry.all_of<T>(m_entity) && "Entities can't have two or more components of the same type.");
		T& component = m_scene->m_registry.emplace<T>(m_entity, std::forward<Args>(args)...);
		component.Init(Entity{m_entity, m_scene });
		component.OnInit();

		return component;
	}

    template <typename T, std::enable_if_t<std::is_empty_v<T>, bool>>
    void Entity::AddComponent()
    {
		assert(IsValid() && "Entity must be valid to add a component.");
		assert(!m_scene->m_registry.all_of<T>(m_entity) && "Entities can't have two or more components of the same type.");
		m_scene->m_registry.emplace<T>(m_entity);
    }

    template <typename T>
	T& Entity::GetComponent() const
	{
		assert(IsValid() && "Entity must be valid to get a component.");
		assert(m_scene->m_registry.all_of<T>(m_entity) && "Entity must have the Component before you get it.");

		return  m_scene->m_registry.get<T>(m_entity);
	}

	template <typename T>
	void Entity::RemoveComponent()
	{
		assert(IsValid() && "Entity must be valid to remove a component.");
		assert(m_scene->m_registry.all_of<T>(m_entity) && "Entity must have the Component before you remove it.");

		m_scene->m_registry.remove<T>();
	}

	template <typename T>
	bool Entity::HasComponent() const
	{
		assert(IsValid() && "Entity must be valid to have a component.");
		return m_scene->m_registry.all_of<T>(m_entity);
	}

	template <typename... T>
	bool Entity::HasAllComponents() const
	{
		assert(IsValid() && "Entity must be valid to have a component.");
		return m_scene->m_registry.all_of<T...>(m_entity);
	}
}

#endif