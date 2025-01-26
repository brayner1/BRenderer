#ifndef BRR_TEXTURESTORAGE_H
#define BRR_TEXTURESTORAGE_H

#include <Core/Storage/ResourceAllocator.h>
#include <Renderer/RenderEnums.h>
#include <Renderer/RenderingResourceIDs.h>
#include <Renderer/GpuResources/GpuResourcesHandles.h>

namespace brr::render
{
    class TextureStorage
    {
    public:

        struct Texture
        {
            Texture2DHandle texture_2d_handle;
            uint32_t width = 0;
            uint32_t height = 0;
            DataFormat image_format = DataFormat::Undefined;
        };

        TextureStorage() = default;

        ~TextureStorage();

        TextureID AllocateTexture();

        void InitTexture(TextureID texture_id, void* data, uint32_t width, uint32_t height, DataFormat image_format);

        void DestroyTexture(TextureID surface_id);

        Texture* GetTexture(TextureID texture_id) const;

    private:

        ResourceAllocator<Texture> m_texture_allocator;
    };
}

#endif