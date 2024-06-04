#ifndef BRR_ENTITY_H
#define BRR_ENTITY_H
#include <Core/Events/Event.h>
#include <Scene/Scene.h>

#include "Core/LogSystem.h"

namespace brr
{
	class EntityComponent;

	template <typename Ty>
	concept ComponentType = std::derived_from<Ty, EntityComponent> && !std::is_same_v<Ty, EntityComponent>;

	template <typename Ty>
	concept EmptyType = std::is_empty_v<Ty>;

    class Entity
	{
	public:
		Entity() = default;

		template <ComponentType T, typename... Args>
		T& AddComponent(Args&&... args);

		template <typename T> requires (EmptyType<T>)
		void AddComponent();

		template<ComponentType T> requires (!EmptyType<T>)
		[[nodiscard]] T& GetComponent() const;

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

		Entity(entt::entity entity_handle, Scene* scene);

		template <ComponentType T>
		void OnComponentDestroyed(entt::registry& registry, entt::entity entity);

		Scene* m_scene = nullptr;
		entt::entity m_entity{ entt::null };
	};

	/**********************
	 *** Implementation ***
	 *********************/

	template <ComponentType T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		assert(IsValid() && "Entity must be valid to add a component.");
		assert(!m_scene->m_registry.all_of<T>(m_entity) && "Entities can't have two or more components of the same type.");
		T& component = m_scene->m_registry.emplace<T>(m_entity, std::forward<Args>(args)...);
		component.m_entity = Entity{m_entity, m_scene };
		component.OnInit();
		if (m_scene->m_scene_render_proxy)
		{
		    component.RegisterGraphics();
		}
		m_scene->m_registry.on_destroy<T>().connect<&Entity::OnComponentDestroyed<T>>(*this);

		return component;
	}

    template <typename T> requires (EmptyType<T>)
    void Entity::AddComponent()
    {
		assert(IsValid() && "Entity must be valid to add a component.");
		assert(!m_scene->m_registry.all_of<T>(m_entity) && "Entities can't have two or more components of the same type.");
		m_scene->m_registry.emplace<T>(m_entity);
    }

    template <ComponentType T> requires (!EmptyType<T>)
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

		if constexpr (!EmptyType<T>)
		{
			T& component = m_scene->m_registry.get<T>(m_entity);
		    if (m_scene->m_scene_render_proxy)
		    {
		        component.UnregisterGraphics();
		    }
			 // TODO: Call this for every component when Scene is destroyed
			component.OnDestroy();
		}
		m_scene->m_registry.remove<T>(m_entity);
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

    template <ComponentType T>
    void Entity::OnComponentDestroyed(entt::registry& registry,
        entt::entity entity)
    {
		T& component = registry.get<T>(entity);
		EventEmitter<>::Emit(component.m_destroyed_event);
    }
}

#endif