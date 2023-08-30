#ifndef BRR_RESOURCEALLOCATOR_H
#define BRR_RESOURCEALLOCATOR_H

#include <Renderer/ResourcesHandles.h>

#include <set>

namespace brr::render
{
    template<typename T>
    class ResourceAllocator
    {
    public:

        ResourceAllocator()
        {}

        ResourceHandle CreateResource();

        bool DestroyResource(const ResourceHandle& handle);

        T* GetResource(const ResourceHandle& handle) const;

    private:

        bool ValidateHandle(const ResourceHandle& handle) const;

        static constexpr uint32_t INVALID_BIT = 0x80000000;

        std::vector<T> m_resources;
        std::vector<uint32_t> m_validation;

        std::set<uint32_t> free_set;
    };

    template <typename T>
    ResourceHandle ResourceAllocator<T>::CreateResource()
    {
        ResourceHandle handle;

        if (free_set.empty())
        {
            m_resources.emplace();
            handle.index =  m_resources.size() - 1;
            handle.validation = 0;
        }
        else
        {
            const uint32_t free_idx = *(free_set.begin());
            free_set.erase(free_set.begin());

            handle.index = free_idx;

            uint32_t& validation = m_validation[free_idx];
            validation &= ~INVALID_BIT;

            handle.validation = validation;
            ++handle.validation;

            m_resources[handle.index] = T();
        }

        return handle;
    }

    template <typename T>
    bool ResourceAllocator<T>::DestroyResource(const ResourceHandle& handle)
    {
        if (!ValidateHandle(handle))
        {
            return false;
        }

        free_set.emplace(handle.index);
        m_validation[handle.index] |= INVALID_BIT;

        return true;
    }

    template <typename T>
    T* ResourceAllocator<T>::GetResource(const ResourceHandle& handle) const
    {
        if (!ValidateHandle(handle))
        {
            return nullptr;
        }

        return &m_resources[handle.index];
    }

    template <typename T>
    bool ResourceAllocator<T>::ValidateHandle(const ResourceHandle& handle) const
    {
        if (handle.index >= m_resources.size())
        {
            BRR_LogError("Invalid resource handle. Index points to non-existent position.");
            return false;
        }

        if (handle.validation & INVALID_BIT)
        {
            BRR_LogError("Invalid handle. Referencing a deleted resource.");
            return false;
        }

        if (handle.validation != m_validation[handle.index])
        {
            BRR_LogError("Invalid handle. Handle validation does not match.");
            return false;
        }

        return true;
    }
}

#endif