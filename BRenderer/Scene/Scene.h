#ifndef BRR_SCENE_H
#define BRR_SCENE_H

#include <Core/thirdpartiesInc.h>
#include <Core/Events/Event.h>

#include <Visualization/SceneRendererProxy.h>

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

		Entity Add3DEntity(std::string entity_name, Transform3DComponent* parent = nullptr);

		void RemoveEntity(Entity entity);

		std::vector<Entity> GetRootEntities() const;

        PerspectiveCameraComponent* GetMainCamera();
		void SetMainCamera(Entity main_camera_entity);

		vis::SceneRenderProxy* GetSceneRendererProxy() const { return m_scene_render_proxy.get(); }

		void Update();

	private:
		
		friend class Entity;
		friend struct NodeComponent;
		template <typename...>
		friend class SceneComponentsView;

		void ParentChanged(NodeComponent* changed_node);

		entt::registry m_registry {};

		std::vector<entt::entity> m_root_nodes {};

		std::unique_ptr<vis::SceneRenderProxy> m_scene_render_proxy {};

		entt::entity m_main_camera;
	};
}

#endif