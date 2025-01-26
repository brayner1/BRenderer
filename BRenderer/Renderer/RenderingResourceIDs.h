#ifndef BRR_RENDERINGRESOURCEIDS_H
#define BRR_RENDERINGRESOURCEIDS_H

#include <Core/Storage/ResourceAllocator.h>

namespace brr::render
{
    struct SurfaceID : public ResourceHandle
    {
        SurfaceID() = default;

        SurfaceID(const ResourceHandle& resource_handle)
        : ResourceHandle(resource_handle)
        {}
    };

    struct TextureID : public ResourceHandle
    {
        TextureID() = default;

        TextureID(const ResourceHandle& resource_handle)
        : ResourceHandle(resource_handle)
        {}
    };
}

#endif