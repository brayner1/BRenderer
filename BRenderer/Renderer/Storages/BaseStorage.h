#ifndef BRR_BAESTORAGE_H
#define BRR_BAESTORAGE_H

#include <Core/Storage/ResourceAllocator.h>

namespace brr::render
{
    template<typename T, typename HandleT = ResourceHandle>
    class BaseStorage
    {
    public:
        BaseStorage() = default;
        ~BaseStorage() = default;
        // Add any common methods or properties that all storages should have
        
        HandleT AllocateResource()
        {
            return m_allocator.AllocateResource();
        }

    protected:

        T* InitResource(HandleT handle, const T& resource)
        {
            return m_allocator.InitializeResource(handle, resource);
        }

        T* InitResource(HandleT handle, T&& resource)
        {
            return m_allocator.InitializeResource(handle, std::move(resource));
        }

        T* GetResource(HandleT handle)
        {
            return m_allocator.GetResource(handle);
        }

        void DestroyResource(HandleT handle)
        {
            m_allocator.DestroyResource(handle);
        }

    private:
        ResourceAllocator<T> m_allocator;

    };
}

#endif