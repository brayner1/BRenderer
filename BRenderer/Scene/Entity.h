#ifndef BRR_ENTITY_H
#define BRR_ENTITY_H
#include "Scene/Scene.h"

namespace brr
{

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity entity_handle, Scene* scene);

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args);

		template<typename T>
		T& GetComponent();

		template<typename T>
		void RemoveComponent();

		template<typename... T>
		[[nodiscard]] bool HasComponent() const;

		bool IsValid() const { return entity_ != entt::null && scene_ != nullptr; }
		operator bool() const { return IsValid(); }

	private:
		friend class Scene;

		entt::entity entity_{ entt::null };
		Scene* scene_ = nullptr;
	};

	/******************
	 * Implementation *
	 ******************/

	template <typename T, typename ... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		assert(IsValid() && "Entity must be valid to add a component.");
		assert(!scene_->m_registry_.all_of<T>(entity_) && "Entities can't have two or more parameters of the same type.");

		return scene_->m_registry_.emplace<T>(entity_, std::forward<Args>(args)...);
	}

	template <typename T>
	T& Entity::GetComponent()
	{
		assert(IsValid() && "Entity must be valid to get a component.");
		assert(scene_->m_registry_.all_of<T>(entity_) && "Entity must have the Component before you get it.");

		return  scene_->m_registry_.get<T>(entity_);
	}

	template <typename T>
	void Entity::RemoveComponent()
	{
		assert(IsValid() && "Entity must be valid to remove a component.");
		assert(scene_->m_registry_.all_of<T>(entity_) && "Entity must have the Component before you remove it.");

		scene_->m_registry_.remove<T>();
	}

	template <typename ... T>
	bool Entity::HasComponent() const
	{
		assert(IsValid() && "Entity must be valid to have a component.");
		return scene_->m_registry_.all_of<T>(entity_);
	}
}

#endif