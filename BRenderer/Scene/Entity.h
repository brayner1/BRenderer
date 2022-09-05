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
		T& AddComponent(Args&&... args)
		{
			assert(!scene_->registry_.all_of<T>(entity_) && "Entities can't have two or more parameters of the same type.");

			return scene_->registry_.emplace<T>(entity_, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			assert(scene_->registry_.all_of<T>(entity_) && "Entity must have the Component before you get it.");

			return  scene_->registry_.get<T>(entity_);
		}

		template<typename T>
		void RemoveComponent()
		{
			assert(scene_->registry_.all_of<T>(entity_) && "Entity must have the Component before you remove it.");

			scene_->registry_.remove<T>();
		}

		template<typename... T>
		[[nodiscard]]
		bool HasComponent() const
		{
			return scene_->registry_.all_of<T>(entity_);
		}

		operator bool() const
		{
			return entity_ != entt::null && scene_ != nullptr;
		}

	private:
		entt::entity entity_{ entt::null };
		Scene* scene_ = nullptr;
	};

}

#endif