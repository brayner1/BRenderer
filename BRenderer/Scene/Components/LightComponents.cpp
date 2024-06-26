#include "LightComponents.h"

#include "Renderer/RenderThread.h"

namespace brr
{
    PointLightComponent::PointLightComponent(const glm::vec3& position, const glm::vec3& color, float intensity) noexcept
    : m_position(position), m_intensity(intensity), m_color(color)
    {
    }

    PointLightComponent::PointLightComponent(PointLightComponent&& other) noexcept
    {
        *this = std::move(other);
    }

    PointLightComponent::~PointLightComponent()
    {
        if (m_light_id != render::LightID::NULL_ID)
        {
            UnregisterGraphics();
        }
    }

    PointLightComponent& PointLightComponent::operator=(PointLightComponent&& other) noexcept
    {
        m_position = other.m_position;
        m_intensity = other.m_intensity;
        m_color = other.m_color;
        m_light_id = other.m_light_id;
        EntityComponent::operator=(std::move(other));

        other.m_light_id = render::LightID::NULL_ID;
        return *this;
    }

    void PointLightComponent::SetPosition(const glm::vec3& position)
    {
        if (m_position != position)
        {
            m_position = position;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdatePointLight(m_light_id, m_position, m_color, m_intensity);
            }
        }
    }

    void PointLightComponent::SetIntensity(float intensity)
    {
        if (m_intensity != intensity)
        {
            m_intensity = intensity;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdatePointLight(m_light_id, m_position, m_color, m_intensity);
            }
        }
    }

    void PointLightComponent::SetColor(const glm::vec3& color)
    {
        if (m_color != color)
        {
            m_color = color;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdatePointLight(m_light_id, m_position, m_color, m_intensity);
            }
        }
    }

    void PointLightComponent::RegisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer_proxy && "Can't call 'PointLightComponent::RegisterGraphics' when SceneRenderer is NULL.");
        m_light_id = scene_renderer_proxy->CreatePointLight(m_position, m_color, m_intensity);
    }

    void PointLightComponent::UnregisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer && "Can't call 'PointLightComponent::UnregisterGraphics' when SceneRenderer is NULL.");
        scene_renderer->DestroyLight(m_light_id);
        m_light_id = render::LightID::NULL_ID;
    }

    DirectionalLightComponent::DirectionalLightComponent(const glm::vec3& direction, const glm::vec3& color,
                                                         float intensity) noexcept
    : m_direction(direction), m_intensity(intensity), m_color(color)
    {
    }

    DirectionalLightComponent::DirectionalLightComponent(DirectionalLightComponent&& other) noexcept
    {
        *this = std::move(other);
    }

    DirectionalLightComponent::~DirectionalLightComponent()
    {
        if (m_light_id != render::LightID::NULL_ID)
        {
            UnregisterGraphics();
        }
    }

    DirectionalLightComponent& DirectionalLightComponent::operator=(DirectionalLightComponent&& other) noexcept
    {
        m_direction = other.m_direction;
        m_intensity = other.m_intensity;
        m_color = other.m_color;
        m_light_id = other.m_light_id;
        EntityComponent::operator=(std::move(other));

        other.m_light_id = render::LightID::NULL_ID;;
        return *this;
    }

    void DirectionalLightComponent::SetDirection(const glm::vec3& direction)
    {
        if (m_direction != direction)
        {
            m_direction = direction;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateDirectionalLight(m_light_id, m_direction, m_color, m_intensity);
            }
        }
    }

    void DirectionalLightComponent::SetIntensity(float intensity)
    {
        if (m_intensity != intensity)
        {
            m_intensity = intensity;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateDirectionalLight(m_light_id, m_direction, m_color, m_intensity);
            }
        }
    }

    void DirectionalLightComponent::SetColor(const glm::vec3& color)
    {
        if (m_color != color)
        {
            m_color = color;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateDirectionalLight(m_light_id, m_direction, m_color, m_intensity);
            }
        }
    }

    void DirectionalLightComponent::RegisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer_proxy && "Can't call 'DirectionalLightComponent::RegisterGraphics' when SceneRenderer is NULL.");
        m_light_id = GetScene()->GetSceneRendererProxy()->CreateDirectionalLight(m_direction, m_color, m_intensity);
    }

    void DirectionalLightComponent::UnregisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer && "Can't call 'DirectionalLightComponent::UnregisterGraphics' when SceneRenderer is NULL.");
        scene_renderer->DestroyLight(m_light_id);
        m_light_id = render::LightID::NULL_ID;
    }

    SpotLightComponent::SpotLightComponent(const glm::vec3& position, const glm::vec3& direction, float cuttof_angle,
                                           const glm::vec3 color, float intensity) noexcept
    : m_position(position), m_cutoff_angle(cuttof_angle), m_direction(direction), m_intensity(intensity),
      m_color(color)
    {
    }

    SpotLightComponent::SpotLightComponent(SpotLightComponent&& other) noexcept
    {
        *this = std::move(other);
    }

    SpotLightComponent::~SpotLightComponent()
    {
        if (m_light_id != render::LightID::NULL_ID)
        {
            UnregisterGraphics();
        }
    }

    SpotLightComponent& SpotLightComponent::operator=(SpotLightComponent&& other) noexcept
    {
        m_position = other.m_position;
        m_cutoff_angle = other.m_cutoff_angle;
        m_direction = other.m_direction;
        m_intensity = other.m_intensity;
        m_color = other.m_color;
        m_light_id = other.m_light_id;
        EntityComponent::operator=(std::move(other));

        other.m_light_id = render::LightID::NULL_ID;
        return *this;
    }

    void SpotLightComponent::SetPosition(const glm::vec3& position)
    {
        if (m_position != position)
        {
            m_position = position;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
            }
        }
    }

    void SpotLightComponent::SetDirection(const glm::vec3& direction)
    {
        if (m_direction != direction)
        {
            m_direction = direction;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
            }
        }
    }

    void SpotLightComponent::SetCutoffAngle(float cutoff_angle)
    {
        if (m_cutoff_angle != cutoff_angle)
        {
            m_cutoff_angle = cutoff_angle;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
            }
        }
    }

    void SpotLightComponent::SetIntensity(float intensity)
    {
        if (m_intensity != intensity)
        {
            m_intensity = intensity;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
            }
        }
    }

    void SpotLightComponent::SetColor(const glm::vec3& color)
    {
        if (m_color != color)
        {
            m_color = color;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
            }
        }
    }

    void SpotLightComponent::RegisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer_proxy && "Can't call 'SpotLightComponent::RegisterGraphics' when SceneRenderer is NULL.");
        m_light_id = GetScene()->GetSceneRendererProxy()->CreateSpotLight(m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
    }

    void SpotLightComponent::UnregisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer && "Can't call 'SpotLightComponent::UnregisterGraphics' when SceneRenderer is NULL.");
        scene_renderer->DestroyLight(m_light_id);
        m_light_id = render::LightID::NULL_ID;
    }

    AmbientLightComponent::AmbientLightComponent(const glm::vec3& color, float intensity)
    : m_color(color), m_intensity(intensity)
    {
    }

    AmbientLightComponent::AmbientLightComponent(AmbientLightComponent&& other) noexcept
    {
        *this = std::move(other);
    }

    AmbientLightComponent::~AmbientLightComponent()
    {
        if (m_light_id != render::LightID::NULL_ID)
        {
            UnregisterGraphics();
        }
    }

    AmbientLightComponent& AmbientLightComponent::operator=(AmbientLightComponent&& other) noexcept
    {
        m_intensity = other.m_intensity;
        m_color = other.m_color;
        m_light_id = other.m_light_id;
        EntityComponent::operator=(std::move(other));

        other.m_light_id = render::LightID::NULL_ID;
        return *this;
    }

    void AmbientLightComponent::SetColor(const glm::vec3& color)
    {
        if (m_color != color)
        {
            m_color = color;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateAmbientLight(m_light_id, m_color, m_intensity);
            }
        }
    }

    void AmbientLightComponent::SetIntensity(float intensity)
    {
        if (m_intensity != intensity)
        {
            m_intensity = intensity;
            if (vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy())
            {
                scene_renderer->UpdateAmbientLight(m_light_id, m_color, m_intensity);
            }
        }
    }

    void AmbientLightComponent::RegisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer_proxy && "Can't call 'AmbientLightComponent::RegisterGraphics' when SceneRenderer is NULL.");
        m_light_id = GetScene()->GetSceneRendererProxy()->CreateAmbientLight(m_color, m_intensity);
    }

    void AmbientLightComponent::UnregisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer && "Can't call 'AmbientLightComponent::UnregisterGraphics' when SceneRenderer is NULL.");
        scene_renderer->DestroyLight(m_light_id);
        m_light_id = render::LightID::NULL_ID;
    }
}
