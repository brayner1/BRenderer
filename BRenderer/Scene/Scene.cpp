#include "Scene/Scene.h"

#include <Scene/Components.h>
#include <Scene/Entity.h>
#include <Core/LogSystem.h>
#include <Visualization/SceneRenderer.h>


namespace brr
{
	Scene::Scene()
	{}

	Scene::Scene(PerspectiveCamera* camera) : m_camera(camera)
	{
	}

	Scene::~Scene()
	{
		m_registry.clear();
	}

    void Scene::InitSceneRenderer()
    {
		if (m_scene_renderer)
		{
		    BRR_LogError("Called 'Scene::DestroySceneRenderer', but SceneRenderer is already initialized.");
			return;
		}
		m_scene_renderer = std::make_unique<vis::SceneRenderer>(this);

	    { // Register Transforms
	        auto transform_view = m_registry.view<Transform3DComponent>();
		    for (auto&& [entity, transform] : transform_view.each())
		    {
		        transform.RegisterGraphics();
		    }
	    }
	    { // Register Meshes
	        auto mesh_3d_view = m_registry.view<Mesh3DComponent>();
		    for (auto&& [entity, mesh] : mesh_3d_view.each())
		    {
		        mesh.RegisterGraphics();
		    }
	    }

		// Register lights
		{ // Register Point Lights
		    auto point_light_view = m_registry.view<PointLightComponent>();
		    for (auto&& [entity, point_light] : point_light_view.each())
		    {
		        point_light.RegisterGraphics();
		    }
		}
		{ // Register Directional Lights
		    auto dir_light_view = m_registry.view<DirectionalLightComponent>();
		    for (auto&& [entity, dir_light] : dir_light_view.each())
		    {
		        dir_light.RegisterGraphics();
		    }
		}
		{ // Register Spot Lights
		    auto spot_light_view = m_registry.view<SpotLightComponent>();
		    for (auto&& [entity, spot_light] : spot_light_view.each())
		    {
		        spot_light.RegisterGraphics();
		    }
		}
		{ // Register Ambient Lights
		    auto amb_light_view = m_registry.view<AmbientLightComponent>();
		    for (auto&& [entity, amb_light] : amb_light_view.each())
		    {
		        amb_light.RegisterGraphics();
		    }
		}
    }

    void Scene::DestroySceneRenderer()
    {
		if (!m_scene_renderer)
		{
		    BRR_LogError("Called 'Scene::DestroySceneRenderer', but SceneRenderer is not initialized.");
			return;
		}
		{ // Unregister Transforms
	        auto transform_view = m_registry.view<Transform3DComponent>();
		    for (auto&& [entity, transform] : transform_view.each())
		    {
		        transform.UnregisterGraphics();
		    }
	    }
	    { // Unregister Meshes
	        auto mesh_3d_view = m_registry.view<Mesh3DComponent>();
		    for (auto&& [entity, mesh] : mesh_3d_view.each())
		    {
		        mesh.UnregisterGraphics();
		    }
	    }

		// Unregister lights
		{ // Unregister Point Lights
		    auto point_light_view = m_registry.view<PointLightComponent>();
		    for (auto&& [entity, point_light] : point_light_view.each())
		    {
		        point_light.UnregisterGraphics();
		    }
		}
		{ // Unregister Directional Lights
		    auto dir_light_view = m_registry.view<DirectionalLightComponent>();
		    for (auto&& [entity, dir_light] : dir_light_view.each())
		    {
		        dir_light.UnregisterGraphics();
		    }
		}
		{ // Unregister Spot Lights
		    auto spot_light_view = m_registry.view<SpotLightComponent>();
		    for (auto&& [entity, spot_light] : spot_light_view.each())
		    {
		        spot_light.UnregisterGraphics();
		    }
		}
		{ // Unregister Ambient Lights
		    auto amb_light_view = m_registry.view<AmbientLightComponent>();
		    for (auto&& [entity, amb_light] : amb_light_view.each())
		    {
		        amb_light.UnregisterGraphics();
		    }
		}

		m_scene_renderer.reset();
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
}
