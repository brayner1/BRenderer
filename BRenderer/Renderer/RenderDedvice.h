#ifndef BRR_RENDERDEVICE_H
#define BRR_RENDERDEVICE_H
#include "VkInitializerHelper.h"

namespace brr::render
{

	class RenderDevice
	{
	public:
		static RenderDevice* Get_RenderDevice();

		RenderDevice();

		void Init(Renderer* renderer);

		void CreateShaderFromFilename(std::string file_name);

	private:

		struct ImageResources
		{
			vk::Image m_image;
			vk::ImageView m_image_view;
		};

		static std::unique_ptr<RenderDevice> instance;

		Renderer* renderer = nullptr;

		vk::Device m_pDevice {};
	};

}

#endif