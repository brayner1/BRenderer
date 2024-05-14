#ifndef BRR_RESOURCESHANDLES_H
#define BRR_RESOURCESHANDLES_H

#include <Core/Storage/ResourceAllocator.h>

#include <cstdint>
#include <numeric>

namespace brr::render
{
    struct DescriptorLayoutHandle
    {
    private:
        friend class DescriptorLayoutCache;

        uint32_t m_layout_index;
    };

    struct SwapchainHandle : public ResourceHandle
    {
        SwapchainHandle() = default;

        SwapchainHandle(const ResourceHandle& resource_handle)
        : ResourceHandle(resource_handle)
        {}
    };

    struct BufferHandle : public ResourceHandle
    {
        BufferHandle() = default;

        BufferHandle(const ResourceHandle& resource_handle)
        : ResourceHandle(resource_handle)
        {}
    };

    struct VertexBufferHandle : public ResourceHandle
    {
        VertexBufferHandle() = default;

        VertexBufferHandle(const ResourceHandle& resource_handle)
        : ResourceHandle(resource_handle)
        {}
    };

    struct IndexBufferHandle : public ResourceHandle
    {
        IndexBufferHandle() = default;

        IndexBufferHandle(const ResourceHandle& resource_handle)
        : ResourceHandle(resource_handle)
        {}
    };

    struct Texture2DHandle : public ResourceHandle
    {
        Texture2DHandle() = default;

        Texture2DHandle(const ResourceHandle& resource_handle)
        : ResourceHandle(resource_handle)
        {}
    };

    struct DescriptorSetHandle : public ResourceHandle
    {
        DescriptorSetHandle() = default;

        DescriptorSetHandle(const ResourceHandle& resource_handle)
        : ResourceHandle(resource_handle)
        {}
    };
}

#endif