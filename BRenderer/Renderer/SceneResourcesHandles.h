#ifndef BRR_SCENERESOURCESHANDLES_H
#define BRR_SCENERESOURCESHANDLES_H

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
}

#endif