#ifndef BRR_SCENE_H
#define BRR_SCENE_H
#include "Core/PerspectiveCamera.h"

namespace brr{
	class Entity;
	class Scene
	{
	public:
		Scene();
		Scene(PerspectiveCamera* camera);
		~Scene();

		Entity AddEntity(Entity parent);

		//TODO: Render function API and implementation

		void RenderScene();

	private:
		
		friend class Entity;
		friend class Renderer;

		entt::registry registry_ {};

		PerspectiveCamera* camera_ {};
	};
}

#endif