#ifndef BRR_LIGHTCOMPONENTS_H
#define BRR_LIGHTCOMPONENTS_H

#include <Scene/Components/EntityComponent.h>

namespace brr
{
	struct PointLightComponent : public EntityComponent
	{
		PointLightComponent(const glm::vec3& position, const glm::vec3& color, float intensity) noexcept;
		PointLightComponent(PointLightComponent&& other) noexcept;

		~PointLightComponent();

		PointLightComponent& operator=(PointLightComponent&& other) noexcept;

		void RegisterGraphics();
		void UnregisterGraphics();

	public:

		const glm::vec3& GetPosition() const { return m_position; }
		void SetPosition(const glm::vec3& position);

		float GetIntensity() const { return m_intensity; }
		void SetIntensity(float intensity);

		const glm::vec3& GetColor() const { return m_color; }
		void SetColor(const glm::vec3& color);

	private:
        glm::vec3 m_position;
        float m_intensity;
        glm::vec3 m_color;
        //vis::LightId m_light_id = vis::LightId::NULL_ID;
	};

	struct DirectionalLightComponent : public EntityComponent
	{
	    DirectionalLightComponent(const glm::vec3& direction, const glm::vec3& color, float intensity) noexcept;
		DirectionalLightComponent(DirectionalLightComponent&& other) noexcept;

		~DirectionalLightComponent();

		DirectionalLightComponent& operator=(DirectionalLightComponent&& other) noexcept;

		void RegisterGraphics();
		void UnregisterGraphics();

	public:

		const glm::vec3& GetDirection() const { return m_direction; }
		void SetDirection(const glm::vec3& direction);

		float GetIntensity() const { return m_intensity; }
		void SetIntensity(float intensity);

		const glm::vec3& GetColor() const { return m_color; }
		void SetColor(const glm::vec3& color);

	private:
		glm::vec3 m_direction;
		float m_intensity;
		glm::vec3 m_color;
		//vis::LightId m_light_id = vis::LightId::NULL_ID;
	};

	struct SpotLightComponent : public EntityComponent
	{
	    SpotLightComponent(const glm::vec3& position, const glm::vec3& direction, float cuttof_angle, const glm::vec3 color, float intensity) noexcept;
		SpotLightComponent(SpotLightComponent&& other) noexcept;

		~SpotLightComponent();

		SpotLightComponent& operator=(SpotLightComponent&& other) noexcept;

		void RegisterGraphics();
		void UnregisterGraphics();

	public:

		const glm::vec3& GetPosition() const { return m_position; }
		void SetPosition(const glm::vec3& position);

		const glm::vec3& GetDirection() const { return m_direction; }
		void SetDirection(const glm::vec3& direction);

		float GetCutoffAngle() const { return m_cutoff_angle; }
		void SetCutoffAngle(float cutoff_angle);

		float GetIntensity() const { return m_intensity; }
		void SetIntensity(float intensity);

		const glm::vec3& GetColor() const { return m_color; }
		void SetColor(const glm::vec3& color);

	private:
		glm::vec3 m_position;
		float m_cutoff_angle;
		glm::vec3 m_direction;
		float m_intensity;
		glm::vec3 m_color;
		//vis::LightId m_light_id = vis::LightId::NULL_ID;
	};

	struct AmbientLightComponent : public EntityComponent
	{
	    AmbientLightComponent(const glm::vec3& color, float intensity);
		AmbientLightComponent(AmbientLightComponent&& other) noexcept;

		~AmbientLightComponent();

		AmbientLightComponent& operator=(AmbientLightComponent&& other) noexcept;

		void RegisterGraphics();
		void UnregisterGraphics();

	public:

		const glm::vec3& GetColor() const { return m_color; }
		void SetColor(const glm::vec3& color);

        float GetIntensity() const { return m_intensity; }
		void SetIntensity(float intensity);

	private:

		glm::vec3 m_color;
		float m_intensity;
		//vis::LightId m_light_id = vis::LightId::NULL_ID;
	};
}

#endif