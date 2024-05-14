#ifndef BRR_RESOURCEALLOCATOR_H
#define BRR_RESOURCEALLOCATOR_H
#include <Core/LogSystem.h>

#include <set>

namespace brr
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

        template<typename T > friend class ResourceAllocator;

        uint32_t index;
        uint32_t validation;
    };
    static constexpr ResourceHandle null_handle = ResourceHandle{};

    /**
     * \brief ResourceAllocator is a thread-safe, template allocator for storing non-RAII structures.
     * This means that the destructor of the resource T is not called when destroying a resource.
     * If the resource T have ownership over some data, the data needs to be released before destroying the resource.
     *
     * When creating a resource, a ResourceHandle is generated to represent this unique resource.
     * The resource's pointer (T*) can be obtained using the ResourceHandle.
     * WARNING: After using any non-const function of the allocator, using any resource pointer (T*)
     * previously obtained is undefined behaviour.
     * \tparam T Resource type managed by the ResourceAllocator
     */
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
        std::set<uint32_t> m_free_set;
        mutable std::mutex m_mutex;
    };

    /******************
     * Implementation *
     ******************/

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
        std::lock_guard lock_guard (m_mutex);

        if (m_free_set.empty())
        {
            m_resources.push_back({});
            handle.index =  m_resources.size() - 1;
            handle.validation = m_validation.emplace_back(ValidatorGen::GetNextValidation());
        }
        else
        {
            const uint32_t free_idx = *(m_free_set.begin());
            m_free_set.erase(m_free_set.begin());

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
        std::lock_guard lock_guard (m_mutex);
        if (!ValidateHandle(handle))
        {
            return false;
        }

        m_free_set.emplace(handle.index);
        m_validation[handle.index] |= INVALID_BIT;

        return true;
    }

    template <typename T>
    T* ResourceAllocator<T>::GetResource(const ResourceHandle& handle)
    {
        std::lock_guard lock_guard (m_mutex);
        if (!ValidateHandle(handle))
        {
            return nullptr;
        }

        return &m_resources[handle.index];
    }

    template <typename T>
    bool ResourceAllocator<T>::OwnsResource(const ResourceHandle& handle) const
    {
        std::lock_guard lock_guard (m_mutex);
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