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
     * When creating a resource, a ResourceHandle is returned to represent the new resource.
     * The resource's pointer (T*) can be obtained using the ResourceHandle.
     * \tparam T Resource type managed by the ResourceAllocator
     */
    template<typename T>
    class ResourceAllocator
    {
    public:

        ResourceAllocator(size_t chunk_size = 65536);

        template <typename... Args>
        ResourceHandle CreateResource(T** new_resource_ref = nullptr, Args&&... args);

        bool DestroyResource(const ResourceHandle& handle);

        T* GetResource(const ResourceHandle& handle);

        bool OwnsResource(const ResourceHandle& handle) const;

    private:
        static constexpr size_t TypeSize = sizeof(T);

        bool ValidateHandle(const ResourceHandle& handle) const;

        static constexpr uint32_t INVALID_BIT = 0x80000000;

        std::vector<T*> m_chunks;
        std::vector<uint32_t> m_validation;
        std::set<uint32_t> m_free_set;
        uint32_t m_allocated_elements;
        const uint32_t m_elements_in_chunk;
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
    ResourceAllocator<T>::ResourceAllocator(size_t chunk_size)
    : m_elements_in_chunk(sizeof(T) > chunk_size ? 1 : chunk_size / sizeof(T))
    {
        
        std::allocator<T> allocator;
        T* first_chunk = allocator.allocate(m_elements_in_chunk * sizeof(T));
        m_chunks.push_back(first_chunk);

        auto new_range = std::ranges::iota_view{0u, m_elements_in_chunk};
        m_free_set.insert(new_range.begin(), new_range.end());

        m_validation = std::vector<uint32_t> (m_elements_in_chunk, INVALID_BIT);
        m_allocated_elements = m_elements_in_chunk;
    }

    template <typename T>
    template <typename... Args>
    ResourceHandle ResourceAllocator<T>::CreateResource(T** new_resource_ref, Args&&... args)
    {
        ResourceHandle handle;
        std::lock_guard lock_guard (m_mutex);

        if (m_free_set.empty())
        {
            uint32_t previous_size = m_allocated_elements;

            std::allocator<T> allocator;
            T* new_chunk = allocator.allocate(m_elements_in_chunk * sizeof(T));
            m_chunks.push_back(new_chunk);
            m_allocated_elements += m_elements_in_chunk;

            auto new_range = std::ranges::iota_view{previous_size, m_allocated_elements};
            m_free_set.insert(new_range.begin(), new_range.end());

            m_validation.resize(m_allocated_elements, INVALID_BIT);
        }

        const uint32_t free_idx = *(m_free_set.begin());
        m_free_set.erase(m_free_set.begin());

        const uint32_t chunk_idx = free_idx / m_elements_in_chunk;
        const uint32_t element_idx = free_idx % m_elements_in_chunk;

        handle.index = free_idx;

        handle.validation = m_validation[free_idx] = ValidatorGen::GetNextValidation();
        assert((handle.validation & INVALID_BIT) == 0 && "Validation value can't contain INVALID_BIT");

        T* resource = m_chunks[chunk_idx] + element_idx;
        std::construct_at(resource, std::forward<Args>(args)...);

        if (new_resource_ref)
        {
            *new_resource_ref = resource;
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
            const uint32_t chunk_idx = handle.index / m_elements_in_chunk;
            const uint32_t element_idx = handle.index % m_elements_in_chunk;
            T* resource = m_chunks[chunk_idx] + element_idx;
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

        const uint32_t chunk_idx = handle.index / m_elements_in_chunk;
        const uint32_t element_idx = handle.index % m_elements_in_chunk;

        return m_chunks[chunk_idx] + element_idx;
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
        if (handle.index >= m_allocated_elements)
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