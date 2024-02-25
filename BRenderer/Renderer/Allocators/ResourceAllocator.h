#ifndef BRR_RESOURCEALLOCATOR_H
#define BRR_RESOURCEALLOCATOR_H
#include <Core/LogSystem.h>
#include <Renderer/ResourcesHandles.h>

#include <set>

namespace brr::render
{
    template<typename T>
    class ResourceAllocator
    {
    public:

        ResourceAllocator() = default;

        ResourceHandle CreateResource(T** new_resource_ref = nullptr);

        bool DestroyResource(const ResourceHandle& handle);

        T* GetResource(const ResourceHandle& handle);

        bool OwnsResource(const ResourceHandle& handle) const;

    private:

        bool ValidateHandle(const ResourceHandle& handle) const;

        static constexpr uint32_t INVALID_BIT = 0x80000000;

        std::vector<T> m_resources;
        std::vector<uint32_t> m_validation;

        std::set<uint32_t> free_set;
    };

    namespace
    {
        class ValidatorGen
        {
        public:
            static uint32_t GetNextValidation()
            {
                return curr_validation++;
            }

        private:
            static std::atomic<uint32_t> curr_validation;
        };

        std::atomic<uint32_t> ValidatorGen::curr_validation = 0;
    }

    template <typename T>
    ResourceHandle ResourceAllocator<T>::CreateResource(T** new_resource_ref)
    {
        ResourceHandle handle;

        if (free_set.empty())
        {
            m_resources.push_back({});
            handle.index =  m_resources.size() - 1;
            handle.validation = m_validation.emplace_back(ValidatorGen::GetNextValidation());
        }
        else
        {
            const uint32_t free_idx = *(free_set.begin());
            free_set.erase(free_set.begin());

            handle.index = free_idx;

            handle.validation = m_validation[free_idx] = ValidatorGen::GetNextValidation();

            m_resources[handle.index] = T();
        }

        if (new_resource_ref)
        {
            *new_resource_ref = &m_resources[handle.index];
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
    T* ResourceAllocator<T>::GetResource(const ResourceHandle& handle)
    {
        if (!ValidateHandle(handle))
        {
            return nullptr;
        }

        return &m_resources[handle.index];
    }

    template <typename T>
    bool ResourceAllocator<T>::OwnsResource(const ResourceHandle& handle) const
    {
        if (handle.index >= m_resources.size() 
         || handle.validation != m_validation[handle.index])
        {
            return false;
        }

        return true;
    }

    template <typename T>
    bool ResourceAllocator<T>::ValidateHandle(const ResourceHandle& handle) const
    {
        if (handle.index >= m_resources.size())
        {
            BRR_LogError(
                "Invalid resource handle. Index points to non-existent resource.\n\tHandle index:\t{}\n\tResources count:\t{}",
                handle.index, m_resources.size());
            return false;
        }

        if (handle.validation & INVALID_BIT)
        {
            BRR_LogError("Invalid handle. Handle References a deleted resource.");
            return false;
        }

        if (handle.validation != m_validation[handle.index])
        {
            BRR_LogError(
                "Invalid handle. Handle validation does not match.\n\tHandle validation:\t{}\n\tResource validation\t{}",
                handle.validation, m_validation[handle.index]);
            return false;
        }

        return true;
    }
}

#endif