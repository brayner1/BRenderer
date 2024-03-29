#include "Image.h"

#include <Core/LogSystem.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace brr::vis
{
    Image::Image(std::filesystem::path image_path)
    {
        if (!image_path.has_filename())
        {
            BRR_LogError("Input path does not point to a file.");
            return;
        }

        if (!std::filesystem::exists(image_path))
        {
            BRR_LogError("Invalid input file path '{}'. File does not exist.", image_path.string());
            BRR_LogInfo("Current path: {}", std::filesystem::current_path().string());
            return;
        }

        int w, h, c;
        if (!stbi_info(image_path.string().c_str(), &w, &h, &c))
        {
            BRR_LogError("Input file is not supported by image loading library");
            return;
        }

        m_buffer = stbi_load(image_path.string().c_str(), &w, &h, &c, 4); // force RGBA format for now
        m_width = w;
        m_height = h;
        m_totalSize = w * h * 4;
    }

    Image::Image(uint32_t width, uint32_t height, uint8_t* data)
    {
        if (!data)
        {
            BRR_LogError("Image input data is NULL.");
            return;
        }

        m_buffer = data;
        m_width = width;
        m_height = height;
        m_totalSize = m_width * m_height * 4;
    }

    Image::~Image()
    {
        stbi_image_free(m_buffer);
    }
}
