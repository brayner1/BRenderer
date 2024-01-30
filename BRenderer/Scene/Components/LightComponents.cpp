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
}
