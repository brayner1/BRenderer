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

        explicit ResourceHandle(uint64_t handle_value)
        {
            index = static_cast<uint32_t>(handle_value & 0xFFFFFFFF);
            validation = static_cast<uint32_t>((handle_value >> 32) & 0xFFFFFFFF);
        }

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

        constexpr operator uint64_t() const
        {
            return (static_cast<uint64_t>(validation) << 32) | index;
        }

    protected:

        template<typename T> friend class ResourceAllocator;
        friend struct std::hash<ResourceHandle>;

        uint32_t index;
        uint32_t validation;
    };
    static constexpr ResourceHandle null_handle = ResourceHandle{};

    /**
     * \brief ResourceAllocator is a thread-safe, template allocator for storing resources.
     *
     * When creating a resource, a ResourceHandle is returned to represent the new resource.
     * The resource's pointer (T*) can be obtained using the ResourceHandle.
     * A resource's pointer remains valid until the resource is destroyed.
     * \tparam T Resource type managed by the ResourceAllocator
     */
    template<typename T>
    class ResourceAllocator
    {
    public:

        /**
         * Default constructor
         * @param chunk_size Size of each data chunk.
         */
        ResourceAllocator(size_t chunk_size = 65536);

        /**
         * Allocate a resource but don't initialize it.
         * @return handle for uninitialized allocated resource
         */
        ResourceHandle AllocateResource();

        /**
         * Initialize (construct) allocated resource.
         * @tparam Args List of parameters for resource construction.
         * @param handle Resource handle.
         * @param args Parameters for initializing resource.
         * @return Resource pointer. Returns `nullptr` if handle is not valid.
         */
        template <typename... Args>
        T* InitializeResource(const ResourceHandle& handle, Args&&... args);

        /**
         * Allocate and initialize a new resource.
         * @tparam Args List of parameters for resource construction.
         * @param new_resource_ref Pointer to a pointer of the resource. If not `nullptr`, will be assigned with the new resource pointer.
         * @param args Parameters for initializing resource.
         * @return Handle of the new resource.
         */
        template <typename... Args>
        ResourceHandle CreateResource(T** new_resource_ref = nullptr, Args&&... args);

        /**
         * Destroy the resource owned by the passed handle.
         * @param handle Resource handle.
         * @return `true` if resource was succesfully destroyed. `false` otherwise.
         */
        bool DestroyResource(const ResourceHandle& handle);

        /**
         * Get a pointer to the resource owned by the passed handle.
         * @param handle Resource handle.
         * @return Resource pointer. `nullptr` if handle is not valid.
         */
        T* GetResource(const ResourceHandle& handle) const;

        /**
         * Check if allocator owns resource.
         * @param handle Resource handle.
         * @return `true` if allocator owns resource with the passed handle. `false` otherwise.
         */
        bool OwnsResource(const ResourceHandle& handle) const;

    private:
        static constexpr size_t TypeSize = sizeof(T);

        ResourceHandle _AllocateResource();

        template <typename... Args>
        T* _InitializeResource(const ResourceHandle& handle, Args&&... args);

        bool OwnsHandle(const ResourceHandle& handle) const;

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
    ResourceHandle ResourceAllocator<T>::AllocateResource()
    {
        std::lock_guard lock_guard (m_mutex);
        return _AllocateResource();
    }

    template <typename T>
    template <typename ... Args>
    T* ResourceAllocator<T>::InitializeResource(const ResourceHandle& handle, Args&&... args)
    {
        std::lock_guard lock_guard (m_mutex);
        if (!OwnsHandle(handle))
        {
            return nullptr;
        }
        return _InitializeResource(handle, std::forward<Args>(args)...);
    }

    template <typename T>
    template <typename... Args>
    ResourceHandle ResourceAllocator<T>::CreateResource(T** new_resource_ref, Args&&... args)
    {
        std::lock_guard lock_guard (m_mutex);
        ResourceHandle handle = _AllocateResource();

        T* resource = _InitializeResource(handle, std::forward<Args>(args)...);
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
        if (!OwnsHandle(handle))
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
    T* ResourceAllocator<T>::GetResource(const ResourceHandle& handle) const
    {
        std::lock_guard lock_guard (m_mutex);
        if (!OwnsHandle(handle))
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
        return OwnsHandle(handle);
    }

    template <typename T>
    ResourceHandle ResourceAllocator<T>::_AllocateResource()
    {
        ResourceHandle handle;

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

        handle.index = free_idx;

        handle.validation = m_validation[free_idx] = ValidatorGen::GetNextValidation();
        assert((handle.validation & INVALID_BIT) == 0 && "Validation value can't contain INVALID_BIT");

        return handle;
    }

    template <typename T>
    template <typename ... Args>
    T* ResourceAllocator<T>::_InitializeResource(const ResourceHandle& handle, Args&&... args)
    {
        const uint32_t chunk_idx = handle.index / m_elements_in_chunk;
        const uint32_t element_idx = handle.index % m_elements_in_chunk;

        T* resource = m_chunks[chunk_idx] + element_idx;
        std::construct_at(resource, std::forward<Args>(args)...);

        return resource;
    }

    template <typename T>
    bool ResourceAllocator<T>::OwnsHandle(const ResourceHandle& handle) const
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

template <>
struct std::hash<brr::ResourceHandle>
{
    [[nodiscard]] size_t operator()(const brr::ResourceHandle& resource) const noexcept {
        return std::hash<uint64_t>()(resource);
    }
};

#endif