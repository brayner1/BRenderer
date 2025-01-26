#include "TextureStorage.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>

using namespace brr;
using namespace brr::render;

TextureStorage::~TextureStorage()
{
}

TextureID TextureStorage::AllocateTexture()
{
    return m_texture_allocator.AllocateResource();
}

void TextureStorage::InitTexture(TextureID texture_id, 
                                 void* data,
                                 uint32_t width,
                                 uint32_t height,
                                 DataFormat image_format)
{
    Texture* texture = m_texture_allocator.InitializeResource(texture_id, Texture());

    if (!texture)
    {
        BRR_LogError("Trying to initialize Texture (ID: {}) that does not exist.", static_cast<uint64_t>(texture_id));
        return;
    }

    BRR_LogDebug("Initializing new Texture (ID: {}).", static_cast<uint64_t>(texture_id));

    ImageUsage usage = ImageUsage::SampledImage | ImageUsage::TransferDstImage;

    texture->width = width;
    texture->height = height;
    texture->image_format = image_format;
    VKRD* render_device = VKRD::GetSingleton();
    texture->texture_2d_handle = render_device->Create_Texture2D(width, height, usage, image_format);
    if (data)
    {
        size_t format_byte_size = GetDataFormatByteSize(image_format);
        size_t buffer_size = (size_t)width * height * format_byte_size;
        render_device->UpdateTexture2DData(texture->texture_2d_handle, data, buffer_size, {0, 0}, {width, height});
    }
}

void TextureStorage::DestroyTexture(TextureID texture_id)
{
    if (Texture* texture = m_texture_allocator.GetResource(texture_id))
    {
        VKRD::GetSingleton()->DestroyTexture2D(texture->texture_2d_handle);
        m_texture_allocator.DestroyResource(texture_id);
    }
}

TextureStorage::Texture* TextureStorage::GetTexture(TextureID texture_id) const
{
    return m_texture_allocator.GetResource(texture_id);
}
