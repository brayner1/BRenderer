#ifndef BRR_SCENE_H
#define BRR_SCENE_H

namespace brr{
	class Entity;
	class Scene
	{
	public:

		Scene();
		~Scene();

		Entity AddEntity();

	private:
		
		friend class Entity;

		entt::registry registry_ {};
	};
}

#endif