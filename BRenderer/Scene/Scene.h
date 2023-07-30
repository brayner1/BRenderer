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

		PerspectiveCamera* GetMainCamera() const { return m_camera_.get(); }

	private:
		
		friend class Entity;
		friend class vis::SceneRenderer;

		entt::registry m_registry_ {};

		std::unique_ptr<PerspectiveCamera> m_camera_ {};
	};
}

#endif