#ifndef BRR_SCENE_H
#define BRR_SCENE_H
#include "Renderer/SceneRenderer.h"

#include "Core/PerspectiveCamera.h"

namespace brr{
	namespace render
	{
		class Renderer;
	}
	class Entity;
	class Scene
	{
	public:
		Scene();
		Scene(PerspectiveCamera* camera);
		~Scene();

		Entity AddEntity(Entity parent);

		PerspectiveCamera* GetMainCamera() const { return m_camera_.get(); }

	private:
		
		friend class Entity;
		friend class render::Renderer;

		entt::registry m_registry_ {};

		render::SceneRenderer m_scene_renderer;

		std::unique_ptr<PerspectiveCamera> m_camera_ {};
	};
}

#endif