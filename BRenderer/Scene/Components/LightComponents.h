#ifndef BRR_LIGHTCOMPONENTS_H
#define BRR_LIGHTCOMPONENTS_H

#include <Scene/Components/EntityComponent.h>

#include "Visualization/SceneRenderer.h"

namespace brr
{

	struct PointLightComponent : public EntityComponent
	{
		PointLightComponent(const glm::vec3& position, const glm::vec3& color, float intensity);

		void OnInit();

	private:
        glm::vec3 m_position;
        float m_intensity;
        glm::vec3 m_color;
        vis::LightId m_light_id = vis::LightId::NULL_ID;
	};
}

#endif