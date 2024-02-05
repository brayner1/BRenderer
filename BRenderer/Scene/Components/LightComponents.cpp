#include "LightComponents.h"

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
        if (m_light_id != vis::LightId::NULL_ID)
        {
            GetScene()->GetSceneRenderer()->RemoveLight(m_light_id);
            m_light_id = vis::LightId::NULL_ID;
        }
    }

    PointLightComponent& PointLightComponent::operator=(PointLightComponent&& other) noexcept
    {
        EntityComponent::operator=(std::move(other));
        m_position = other.m_position;
        m_intensity = other.m_intensity;
        m_color = other.m_color;
        m_light_id = other.m_light_id;

        other.m_light_id = vis::LightId::NULL_ID;
        return *this;
    }

    void PointLightComponent::OnInit()
    {
        m_light_id = GetScene()->GetSceneRenderer()->CreatePointLight(m_position, m_color, m_intensity);
    }

    void PointLightComponent::SetPosition(const glm::vec3& position)
    {
        if (m_position != position)
        {
            m_position = position;
            GetScene()->GetSceneRenderer()->UpdatePointLight(m_light_id, m_position, m_color, m_intensity);
        }
    }

    void PointLightComponent::SetIntensity(float intensity)
    {
        if (m_intensity != intensity)
        {
            m_intensity = intensity;
            GetScene()->GetSceneRenderer()->UpdatePointLight(m_light_id, m_position, m_color, m_intensity);
        }
    }

    void PointLightComponent::SetColor(const glm::vec3& color)
    {
        if (m_color != color)
        {
            m_color = color;
            GetScene()->GetSceneRenderer()->UpdatePointLight(m_light_id, m_position, m_color, m_intensity);
        }
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
        if (m_light_id != vis::LightId::NULL_ID)
        {
            GetScene()->GetSceneRenderer()->RemoveLight(m_light_id);
            m_light_id = vis::LightId::NULL_ID;
        }
    }

    DirectionalLightComponent& DirectionalLightComponent::operator=(DirectionalLightComponent&& other) noexcept
    {
        EntityComponent::operator=(std::move(other));
        m_direction = other.m_direction;
        m_intensity = other.m_intensity;
        m_color = other.m_color;
        m_light_id = other.m_light_id;

        other.m_light_id = vis::LightId::NULL_ID;
        return *this;
    }

    void DirectionalLightComponent::OnInit()
    {
        m_light_id = GetScene()->GetSceneRenderer()->CreateDirectionalLight(m_direction, m_color, m_intensity);
    }

    void DirectionalLightComponent::SetDirection(const glm::vec3& direction)
    {
        if (m_direction != direction)
        {
            m_direction = direction;
            GetScene()->GetSceneRenderer()->UpdateDirectionalLight(m_light_id, m_direction, m_color, m_intensity);
        }
    }

    void DirectionalLightComponent::SetIntensity(float intensity)
    {
        if (m_intensity != intensity)
        {
            m_intensity = intensity;
            GetScene()->GetSceneRenderer()->UpdateDirectionalLight(m_light_id, m_direction, m_color, m_intensity);
        }
    }

    void DirectionalLightComponent::SetColor(const glm::vec3& color)
    {
        if (m_color != color)
        {
            m_color = color;
            GetScene()->GetSceneRenderer()->UpdateDirectionalLight(m_light_id, m_direction, m_color, m_intensity);
        }
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
        if (m_light_id != vis::LightId::NULL_ID)
        {
            GetScene()->GetSceneRenderer()->RemoveLight(m_light_id);
            m_light_id = vis::LightId::NULL_ID;
        }
    }

    SpotLightComponent& SpotLightComponent::operator=(SpotLightComponent&& other) noexcept
    {
        EntityComponent::operator=(std::move(other));
        m_position = other.m_position;
        m_cutoff_angle = other.m_cutoff_angle;
        m_direction = other.m_direction;
        m_intensity = other.m_intensity;
        m_color = other.m_color;
        m_light_id = other.m_light_id;

        other.m_light_id = vis::LightId::NULL_ID;
        return *this;
    }

    void SpotLightComponent::OnInit()
    {
        m_light_id = GetScene()->GetSceneRenderer()->CreateSpotLight(m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
    }

    void SpotLightComponent::SetPosition(const glm::vec3& position)
    {
        if (m_position != position)
        {
            m_position = position;
            GetScene()->GetSceneRenderer()->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
        }
    }

    void SpotLightComponent::SetDirection(const glm::vec3& direction)
    {
        if (m_direction != direction)
        {
            m_direction = direction;
            GetScene()->GetSceneRenderer()->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
        }
    }

    void SpotLightComponent::SetCutoffAngle(float cutoff_angle)
    {
        if (m_cutoff_angle != cutoff_angle)
        {
            m_cutoff_angle = cutoff_angle;
            GetScene()->GetSceneRenderer()->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
        }
    }

    void SpotLightComponent::SetIntensity(float intensity)
    {
        if (m_intensity != intensity)
        {
            m_intensity = intensity;
            GetScene()->GetSceneRenderer()->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
        }
    }

    void SpotLightComponent::SetColor(const glm::vec3& color)
    {
        if (m_color != color)
        {
            m_color = color;
            GetScene()->GetSceneRenderer()->UpdateSpotLight(m_light_id, m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
        }
    }
}
