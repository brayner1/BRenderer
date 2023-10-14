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

		Entity Add3DEntity(Entity parent);

		PerspectiveCamera* GetMainCamera() const { return m_camera.get(); }


		void SetSceneRenderer(vis::SceneRenderer* scene_renderer) { m_scene_renderer = scene_renderer; }
		vis::SceneRenderer* GetSceneRenderer() const { return m_scene_renderer; }

	private:
		
		friend class Entity;
		friend class vis::SceneRenderer;

		entt::registry m_registry {};

		vis::SceneRenderer* m_scene_renderer = nullptr;
		std::unique_ptr<PerspectiveCamera> m_camera {};
	};
}

#endif