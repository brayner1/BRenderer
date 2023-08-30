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

        bool operator ==(const ResourceHandle& other) const
        {
            return index == other.index && validation == other.validation;
        }

        bool IsValid() const
        {
            return *this != ResourceHandle();
        }

        operator bool() const
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
    {};

    struct VertexBufferHandle : public ResourceHandle
    {};

    struct IndexBufferHandle : public ResourceHandle
    {};

    struct Texture2DHandle : public ResourceHandle
    {};

    static constexpr ResourceHandle null_handle = ResourceHandle{};
}

#endif