#include "LightComponents.h"

namespace brr
{
    PointLightComponent::PointLightComponent(const glm::vec3& position, const glm::vec3& color, float intensity)
    : m_position(position), m_intensity(intensity), m_color(color)
    {
    }

    void PointLightComponent::OnInit()
    {
        m_light_id = GetScene()->GetSceneRenderer()->CreatePointLight(m_position, m_color, m_intensity);
    }

    DirectionalLightComponent::DirectionalLightComponent(const glm::vec3& direction, const glm::vec3& color,
        float intensity)
    : m_direction(direction), m_intensity(intensity), m_color(color)
    {
    }

    void DirectionalLightComponent::OnInit()
    {
        m_light_id = GetScene()->GetSceneRenderer()->CreateDirectionalLight(m_direction, m_color, m_intensity);
    }

    SpotLightComponent::SpotLightComponent(const glm::vec3& position, const glm::vec3& direction, float cuttof_angle,
                                           const glm::vec3 color, float intensity)
    : m_position(position), m_cutoff_angle(cuttof_angle), m_direction(direction), m_intensity(intensity),
      m_color(color)
    {
    }

    void SpotLightComponent::OnInit()
    {
        m_light_id = GetScene()->GetSceneRenderer()->CreateSpotLight(m_position, m_cutoff_angle, m_direction, m_intensity, m_color);
    }
}
