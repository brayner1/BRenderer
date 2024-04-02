#ifndef BRR_SCENE_H
#define BRR_SCENE_H

#include <Core/thirdpartiesInc.h>
#include <Core/PerspectiveCamera.h>

namespace brr
{
	class Entity;
	class Scene
	{
	public:
		Scene();
		Scene(PerspectiveCamera* camera);
		~Scene();

		void InitSceneRenderer();
		void DestroySceneRenderer();

		Entity Add3DEntity(Entity parent);

		void RemoveEntity(Entity entity);

        [[nodiscard]] PerspectiveCamera* GetMainCamera() const { return m_camera.get(); }

		//vis::SceneRenderer* GetSceneRenderer() const { return m_scene_renderer.get(); }

	private:
		
		friend class Entity;

		entt::registry m_registry {};

	    std::unique_ptr<PerspectiveCamera> m_camera {};
	};
}

#endif