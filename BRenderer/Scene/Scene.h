#ifndef BRR_SCENE_H
#define BRR_SCENE_H

#include <Core/thirdpartiesInc.h>
#include <Core/PerspectiveCamera.h>

#include "Visualization/SceneRendererProxy.h"

namespace brr
{
    class PerspectiveCameraComponent;
    class Entity;
	class Scene
	{
	public:
		Scene();
		~Scene();

		void InitSceneRenderer();
		void DestroySceneRenderer();

		Entity Add3DEntity(Entity parent);

		void RemoveEntity(Entity entity);

        PerspectiveCameraComponent* GetMainCamera();
		void SetMainCamera(Entity main_camera_entity);

		vis::SceneRenderProxy* GetSceneRendererProxy() const { return m_scene_render_proxy.get(); }

		void Update();

	private:
		
		friend class Entity;

		entt::registry m_registry {};

		std::unique_ptr<vis::SceneRenderProxy> m_scene_render_proxy {};

		entt::entity m_main_camera;
	};
}

#endif