#ifndef BRR_RESOURCEALLOCATOR_H
#define BRR_RESOURCEALLOCATOR_H
#include <ranges>
#include <Core/LogSystem.h>

#include <set>
#include <mutex>

namespace brr
{
    template<typename T>
    class ResourceAllocator;

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

        template<typename T> friend class ResourceAllocator;

        uint32_t index;
        uint32_t validation;
    };
    static constexpr ResourceHandle null_handle = ResourceHandle{};

    /**
     * \brief ResourceAllocator is a thread-safe, template allocator for storing resources.
     *
     * When creating a resource, a ResourceHandle is return to represent the new resource.
     * The resource's pointer (T*) can be obtained using the ResourceHandle.
     * WARNING: After using any non-const function of the allocator, using any resource pointer (T*)
     * previously obtained is undefined behaviour.
     * \tparam T Resource type managed by the ResourceAllocator
     */
    template<typename T>
    class ResourceAllocator
    {
    public:

        ResourceAllocator();

        template <typename... Args>
        ResourceHandle CreateResource(T** new_resource_ref = nullptr, Args&&... args);

        bool DestroyResource(const ResourceHandle& handle);

        T* GetResource(const ResourceHandle& handle);

        bool OwnsResource(const ResourceHandle& handle) const;

    private:
        static constexpr size_t TypeSize = sizeof(T);

        bool ValidateHandle(const ResourceHandle& handle) const;

        static constexpr uint32_t INVALID_BIT = 0x80000000;

        T* m_resources;
        size_t m_current_buffer_size;
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
    ResourceAllocator<T>::ResourceAllocator()
    {
        std::allocator<T> allocator;
        m_resources                         = allocator.allocate(32); // Initialize with 32 elements
        m_current_buffer_size               = 32;
        static constexpr auto initial_range = std::ranges::iota_view{0, 32};
        m_free_set.insert(initial_range.begin(), initial_range.end());
        m_validation = std::vector<uint32_t> (32, INVALID_BIT);
    }

    template <typename T>
    template <typename... Args>
    ResourceHandle ResourceAllocator<T>::CreateResource(T** new_resource_ref, Args&&... args)
    {
        ResourceHandle handle;
        std::lock_guard lock_guard (m_mutex);

        if (m_free_set.empty())
        {
            size_t previous_size = m_current_buffer_size;
            T* previous_buffer = m_resources;

            std::allocator<T> allocator;
            m_current_buffer_size = std::ceil(m_current_buffer_size * 1.5f);
            m_resources = allocator.allocate(m_current_buffer_size);
            std::uninitialized_move(previous_buffer, previous_buffer + previous_size, m_resources);
            allocator.deallocate(previous_buffer, previous_size);

            if constexpr (!std::is_trivial_v<T>)
            {
                T* resource = m_resources + previous_size;
                std::construct_at(resource, std::forward<Args>(args)...);
            }

            handle.index =  previous_size;

            m_validation.resize(m_current_buffer_size, INVALID_BIT);
            handle.validation = m_validation[handle.index] = ValidatorGen::GetNextValidation();
            assert((handle.validation & INVALID_BIT) == 0 && "Validation value can't contain INVALID_BIT");

            auto new_range = std::ranges::iota_view{previous_size + 1, m_current_buffer_size};
            m_free_set.insert(new_range.begin(), new_range.end());
        }
        else
        {
            const uint32_t free_idx = *(m_free_set.begin());
            m_free_set.erase(m_free_set.begin());

            handle.index = free_idx;

            handle.validation = m_validation[free_idx] = ValidatorGen::GetNextValidation();
            assert((handle.validation & INVALID_BIT) == 0 && "Validation value can't contain INVALID_BIT");

            if constexpr (!std::is_trivial_v<T>)
            {
                T* resource = m_resources + handle.index;
                std::construct_at(resource, std::forward<Args>(args)...);
            }
        }

        if (new_resource_ref)
        {
            *new_resource_ref = m_resources + handle.index;
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

        if constexpr (!std::is_trivial_v<T> && std::is_destructible_v<T>)
        {
            T* resource = m_resources + handle.index;
            std::destroy_at(resource);
        }

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
        if (!ValidateHandle(handle))
        {
            return false;
        }

        return true;
    }

    template <typename T>
    bool ResourceAllocator<T>::ValidateHandle(const ResourceHandle& handle) const
    {
        if (handle.index >= m_current_buffer_size)
        {
            BRR_LogTrace(
                "Error: Invalid resource handle. Index points to non-existent resource.\n\tHandle index:\t{}\n\tResources count:\t{}",
                handle.index, m_current_buffer_size);
            return false;
        }

        if (handle.validation & INVALID_BIT)
        {
            BRR_LogTrace("Error: Invalid handle. Handle References a deleted resource.");
            return false;
        }

        if (handle.validation != m_validation[handle.index])
        {
            BRR_LogTrace(
                "Error: Invalid handle. Handle validation does not match.\n\tHandle validation:\t{}\n\tResource validation\t{}",
                handle.validation, m_validation[handle.index]);
            return false;
        }

        return true;
    }
}

#endif