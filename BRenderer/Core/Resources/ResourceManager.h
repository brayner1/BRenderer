#ifndef BRR_RESOURCEMANAGER_H
#define BRR_RESOURCEMANAGER_H

#include <Core/UUID.h>
#include <Core/Storage/ResourceAllocator.h>

namespace brr
{
    template<typename T>
	class ResourceRef;

	template<typename T>
	class ResourceManager
	{
	public:

		static ResourceManager<T>& Instance()
		{
			static ResourceManager<T> s_manager_instance {};
			return s_manager_instance;
		}

		ResourceRef<T> GetResource(const std::string& resource_path);
		ResourceRef<T> GetResourceRef(UUID resource_uuid);

		void DestroyResource(const std::string& resource_path);
		void DestroyResource(UUID resource_uuid);

	private:
        friend class ResourceRef<T>;

        class Resource
        {
        public:

            Resource(ResourceHandle resource_handle, UUID uuid = UUID())
            : m_uuid(uuid),
              m_resource_handle(resource_handle),
              m_ref_count(0)
            {}

            Resource(const Resource& other) = delete;

            Resource(Resource&& other) = delete;

            uint32_t GetReferenceCount() const { return m_ref_count; }

            void IncReferenceCount() { ++m_ref_count; }
            void DecReferenceCount() { --m_ref_count; }

            T* GetResource()
            {
                return ResourceManager::Instance().m_allocator.GetResource(m_resource_handle);
            }

            T& operator*()
            {
                return *GetResource();
            }

            T* operator->()
            {
                return GetResource();
            }

        private:
            friend class ResourceManager;
            friend class ResourceRef<T>;

            UUID m_uuid;
            ResourceHandle m_resource_handle;
            uint32_t m_ref_count;
        };

		ResourceManager() {}

        std::unordered_map<UUID, std::string> m_uuid_path_map;
		std::unordered_map<std::string, Resource> m_resource_map;
		ResourceAllocator<T> m_allocator;
	};

    template <typename T>
    class ResourceRef
    {
    public:
        using ResourceType = typename ResourceManager<T>::Resource;

        ResourceRef(ResourceType* resource = nullptr)
        : m_resource(resource)
        {
            if (m_resource)
                m_resource->IncReferenceCount();
        }

        ResourceRef(const ResourceRef& other) noexcept
        {
            *this = other;
        }

        ResourceRef(ResourceRef&& other) noexcept
        {
            *this = std::move(other);
        }

        ResourceRef& operator=(const ResourceRef& other) noexcept
        {
            m_resource = other.m_resource;
            if (m_resource)
                m_resource->IncReferenceCount();
            return *this;
        }

        ResourceRef& operator=(ResourceRef&& other) noexcept
        {
            m_resource = other.m_resource;
            other.m_resource = nullptr;
            return *this;
        }

        ~ResourceRef()
        {
            if (m_resource)
            {
                m_resource->DecReferenceCount();
                if (m_resource->GetReferenceCount() == 0)
                {
                    ResourceManager<T>::Instance().DestroyResource(m_resource->m_uuid);
                }
            }
        }

        operator bool() const
        {
            return m_resource != nullptr;
        }

        T& operator*()
        {
            return **m_resource;
        }

        T* operator->()
        {
            return m_resource->operator->();
        }

    private:
        ResourceType* m_resource = nullptr;
    };

    template <typename T>
    ResourceRef<T> ResourceManager<T>::GetResource(const std::string& resource_path)
    {
        if (!m_resource_map.contains(resource_path))
        {
            ResourceHandle new_resource_handle = m_allocator.CreateResource(nullptr, resource_path);
            auto new_res_iter = m_resource_map.emplace(resource_path, new_resource_handle);
            m_uuid_path_map.emplace(new_res_iter.first->second.m_uuid, resource_path);
            return ResourceRef<T>(&new_res_iter.first->second);
        }
        else
        {
            return ResourceRef<T>(&m_resource_map.at(resource_path));
        }
    }

    template <typename T>
    ResourceRef<T> ResourceManager<T>::GetResourceRef(UUID resource_uuid)
    {
        if (!m_uuid_path_map.contains(resource_uuid))
        {
            BRR_LogError("Can't get non-initialized Resource by UUID.");
            return ResourceRef<T>();
        }

        std::string& resource_path = m_uuid_path_map.at(resource_uuid);
        return GetResource(resource_path);
    }

    template <typename T>
    void ResourceManager<T>::DestroyResource(const std::string& resource_path)
    {
        if (!m_resource_map.contains(resource_path))
        {
            BRR_LogError("Can't destroy non-initialized Resource.");
            return;
        }

        Resource& resource = m_resource_map.at(resource_path);

        m_uuid_path_map.erase(resource.m_uuid);
        m_allocator.DestroyResource(resource.m_resource_handle);
        m_resource_map.erase(resource_path);
    }

    template <typename T>
    void ResourceManager<T>::DestroyResource(UUID resource_uuid)
    {
        if (!m_uuid_path_map.contains(resource_uuid))
        {
            BRR_LogError("Can't destroy non-initialized Resource.");
            return;
        }

        std::string resource_path = m_uuid_path_map.at(resource_uuid);
        DestroyResource(resource_path);
    }

    

}

#endif