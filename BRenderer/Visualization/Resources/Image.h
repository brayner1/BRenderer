#ifndef BRR_IMAGE_H
#define BRR_IMAGE_H

#include <Core/Assets/Asset.h>

#include <filesystem>
#include <Renderer/RenderingResourceIDs.h>

namespace brr::vis
{
    class Image : public Asset
    {
    public:

        // Construct image loaded from file
        Image(std::filesystem::path image_path);

        // Construct image loaded from buffer
        Image(uint32_t width, uint32_t height, uint8_t* data);

        // Destructor
        ~Image() override;

        uint32_t Width() const { return m_width; }
        uint32_t Height() const { return m_height; }

        const uint8_t* Data() const { return m_buffer; }
        uint32_t DataSize() const { return m_totalSize; }

        render::TextureID GetTextureID() const { return m_texture_id; }

    private:
        render::TextureID m_texture_id;
        uint32_t m_totalSize = 0;
        uint32_t m_width = 0, m_height = 0;
        uint8_t* m_buffer = nullptr;
    };

}

#endif