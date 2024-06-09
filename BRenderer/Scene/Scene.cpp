#include "Scene/Scene.h"

#include <iostream>
#include <Scene/Components.h>
#include <Scene/Entity.h>
#include <Core/LogSystem.h>

#include "Components/PerspectiveCameraComponent.h"
#include "Renderer/RenderThread.h"

namespace brr
{
	template <ComponentType T>
	void RegisterComponentGraphics(entt::registry& scene_registry)
	{
	    auto component_view = scene_registry.view<T>();
		for (auto&& [entity, component] : component_view.each())
		{
		    component.RegisterGraphics();
		}
	}

	template <ComponentType T>
	void UnregisterComponentGraphics(entt::registry& scene_registry)
	{
	    auto component_view = scene_registry.view<T>();
		for (auto&& [entity, component] : component_view.each())
		{
		    component.UnregisterGraphics();
		}
	}

	Scene::Scene()
	: m_main_camera(entt::null)
	{}

	Scene::~Scene()
	{
		DestroySceneRenderer();
		m_registry.clear();
	}

    void Scene::InitSceneRenderer()
    {
		if (m_scene_render_proxy)
		{
		    BRR_LogError("Called 'Scene::InitSceneRenderer', but SceneRenderer is already initialized.");
			return;
		}

		m_scene_render_proxy = std::make_unique<vis::SceneRenderProxy>();
		

		// Register Transforms
	    RegisterComponentGraphics<Transform3DComponent>(m_registry);
	    // Register Meshes
	    RegisterComponentGraphics<Mesh3DComponent>(m_registry);
		// Register Cameras
	    RegisterComponentGraphics<PerspectiveCameraComponent>(m_registry);

		/* Register lights */

	    // Register Point Lights
		RegisterComponentGraphics<PointLightComponent>(m_registry);
		// Register Directional Lights
		RegisterComponentGraphics<DirectionalLightComponent>(m_registry);
		// Register Spot Lights
		RegisterComponentGraphics<SpotLightComponent>(m_registry);
		// Register Ambient Lights
		RegisterComponentGraphics<AmbientLightComponent>(m_registry);
    }

    void Scene::DestroySceneRenderer()
    {
		if (!m_scene_render_proxy)
		{
		    BRR_LogError("Called 'Scene::DestroySceneRenderer', but SceneRenderer is not initialized.");
			return;
		}
		// Unregister Transforms
		UnregisterComponentGraphics<Transform3DComponent>(m_registry);
	    // Unregister Meshes
		UnregisterComponentGraphics<Mesh3DComponent>(m_registry);
		// Unregister Cameras
	    UnregisterComponentGraphics<PerspectiveCameraComponent>(m_registry);

		/* Unregister lights */

		// Unregister Point Lights
		UnregisterComponentGraphics<PointLightComponent>(m_registry);
		// Unregister Directional Lights
		UnregisterComponentGraphics<DirectionalLightComponent>(m_registry);
		// Unregister Spot Lights
		UnregisterComponentGraphics<SpotLightComponent>(m_registry);
		// Unregister Ambient Lights
		UnregisterComponentGraphics<AmbientLightComponent>(m_registry);

		m_scene_render_proxy.reset();
    }

    Entity Scene::Add3DEntity(Entity parent)
	{
		BRR_LogInfo("Adding new 3D Entity");
		entt::entity new_entity = m_registry.create();

		Entity new_entity_struct(new_entity, this);

		NodeComponent& node = new_entity_struct.AddComponent<NodeComponent>();

		Transform3DComponent& transform = new_entity_struct.AddComponent<Transform3DComponent>();

		if (parent)
		{
			assert(m_registry.valid(parent.m_entity) && "Parent Entity must be valid.");
			transform.SetParent(&parent.GetComponent<Transform3DComponent>());
		}

		return Entity{new_entity, this};
	}

    void Scene::RemoveEntity(Entity entity)
    {
		if (!m_registry.valid(entity.m_entity))
		{
		    BRR_LogError("Trying to remove invalid entity. Entity Id '{}' is not valid.", uint32_t(entity.m_entity));
			return;
		}

		NodeComponent& node = entity.GetComponent<NodeComponent>();

        if (NodeComponent* parent_node = node.GetParentNode())
		{
		    parent_node->RemoveChild(&node);
		}

		for (NodeComponent* children : node.GetChildren())
		{
		    RemoveEntity(children->GetEntity());
		}

		m_registry.destroy(entity.m_entity);
    }

    PerspectiveCameraComponent* Scene::GetMainCamera()
    {
		if (m_main_camera == entt::null)
		{
		    return nullptr;
		}

		PerspectiveCameraComponent& camera_component = m_registry.get<PerspectiveCameraComponent>(m_main_camera);
		return &camera_component;
    }

    void Scene::SetMainCamera(Entity main_camera_entity)
    {
		if (!main_camera_entity.HasComponent<PerspectiveCameraComponent>())
		{
		    BRR_LogError("Impossible to set as the Scene's main camera an Entity that does not contain a CameraComponent.");
			return;
		}
		m_main_camera = main_camera_entity.m_entity;
    }

    void Scene::Update()
    {
		if (m_scene_render_proxy)
		{
		    m_scene_render_proxy->FlushUpdateCommands();
		}
    }
}
