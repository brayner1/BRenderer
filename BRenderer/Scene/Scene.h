#ifndef BRR_SCENE_H
#define BRR_SCENE_H

#include <Core/thirdpartiesInc.h>
#include <Core/PerspectiveCamera.h>

namespace brr
{
    namespace vis
	{
        class SceneRenderer;
        class WindowRenderer;
	}
	class Entity;
	class Scene
	{
	public:
		Scene();
		Scene(PerspectiveCamera* camera);
		~Scene();

		void InitSceneRenderer();

		Entity Add3DEntity(Entity parent);

		void RemoveEntity(Entity entity);

		PerspectiveCamera* GetMainCamera() const { return m_camera.get(); }

		vis::SceneRenderer* GetSceneRenderer() const { return m_scene_renderer.get(); }

	private:
		
		friend class Entity;
		friend class vis::SceneRenderer;

		entt::registry m_registry {};

		std::unique_ptr<vis::SceneRenderer> m_scene_renderer;
	    std::unique_ptr<PerspectiveCamera> m_camera {};
	};
}

#endif