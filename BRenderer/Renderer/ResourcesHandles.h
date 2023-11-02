#ifndef BRR_RESOURCESHANDLES_H
#define BRR_RESOURCESHANDLES_H

#include <cstdint>
#include <numeric>

namespace brr::render
{
    struct ResourceHandle
    {

        constexpr ResourceHandle() : index(std::numeric_limits<uint32_t>::max()), validation(std::numeric_limits<uint32_t>::max())
        {}

        constexpr bool operator ==(const ResourceHandle& other) const
        {
            return index == other.index && validation == other.validation;
        }

        constexpr bool IsValid() const
        {
            return *this != ResourceHandle();
        }

        constexpr operator bool() const
        {
            return IsValid();
        }

    protected:

        friend class VulkanRenderDevice;
        template<typename T > friend class ResourceAllocator;

        uint32_t index;
        uint32_t validation;
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

    static constexpr ResourceHandle null_handle = ResourceHandle{};
}

#endif