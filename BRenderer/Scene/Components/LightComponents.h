#ifndef BRR_LIGHTCOMPONENTS_H
#define BRR_LIGHTCOMPONENTS_H

#include <Scene/Components/EntityComponent.h>

namespace brr
{
	struct PointLightComponent : public EntityComponent
	{
		PointLightComponent(const glm::vec3& color, float intensity) noexcept;
		PointLightComponent(PointLightComponent&& other) noexcept;

		~PointLightComponent();

		PointLightComponent& operator=(PointLightComponent&& other) noexcept;

	public:

		float GetIntensity() const { return m_intensity; }
		void SetIntensity(float intensity);

		const glm::vec3& GetColor() const { return m_color; }
		void SetColor(const glm::vec3& color);

	public:

		void RegisterGraphics();
		void UnregisterGraphics();

	private:
        glm::vec3 m_color;
        float m_intensity;
        render::LightID m_light_id = render::LightID::NULL_ID;
	};

	struct DirectionalLightComponent : public EntityComponent
	{
	    DirectionalLightComponent(const glm::vec3& color, float intensity) noexcept;
		DirectionalLightComponent(DirectionalLightComponent&& other) noexcept;

		~DirectionalLightComponent();

		DirectionalLightComponent& operator=(DirectionalLightComponent&& other) noexcept;

	public:

		float GetIntensity() const { return m_intensity; }
		void SetIntensity(float intensity);

		const glm::vec3& GetColor() const { return m_color; }
		void SetColor(const glm::vec3& color);

	public:

		void RegisterGraphics();
		void UnregisterGraphics();

	private:
		glm::vec3 m_color;
		float m_intensity;
		render::LightID m_light_id = render::LightID::NULL_ID;
	};

	struct SpotLightComponent : public EntityComponent
	{
	    SpotLightComponent(const glm::vec3& color, float intensity, float cutoff_angle) noexcept;
		SpotLightComponent(SpotLightComponent&& other) noexcept;

		~SpotLightComponent();

		SpotLightComponent& operator=(SpotLightComponent&& other) noexcept;

	public:

		float GetCutoffAngle() const { return m_cutoff_angle; }
		void SetCutoffAngle(float cutoff_angle);

		float GetIntensity() const { return m_intensity; }
		void SetIntensity(float intensity);

		const glm::vec3& GetColor() const { return m_color; }
		void SetColor(const glm::vec3& color);

	public:

		void RegisterGraphics();
		void UnregisterGraphics();

	private:
		glm::vec3 m_color;
		float m_intensity;
		float m_cutoff_angle;
		render::LightID m_light_id = render::LightID::NULL_ID;
	};

	struct AmbientLightComponent : public EntityComponent
	{
	    AmbientLightComponent(const glm::vec3& color, float intensity);
		AmbientLightComponent(AmbientLightComponent&& other) noexcept;

		~AmbientLightComponent();

		AmbientLightComponent& operator=(AmbientLightComponent&& other) noexcept;

	public:

		const glm::vec3& GetColor() const { return m_color; }
		void SetColor(const glm::vec3& color);

        float GetIntensity() const { return m_intensity; }
		void SetIntensity(float intensity);

	public:

		void RegisterGraphics();
		void UnregisterGraphics();

	private:

		glm::vec3 m_color;
		float m_intensity;
		render::LightID m_light_id = render::LightID::NULL_ID;
	};
}

#endif