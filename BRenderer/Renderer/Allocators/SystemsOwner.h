#ifndef BRR_SYSTEMSOWNER_H
#define BRR_SYSTEMSOWNER_H

#include <unordered_map>
#include <memory>

#include <Core/LogSystem.h>

#include "Renderer/Internal/WindowRenderer.h"

namespace brr::render
{
    template <class System>
    class SystemOwner
    {
    public:
        using SystemId = uint64_t;
        using iterator = typename std::unordered_map<SystemId, std::unique_ptr<System>>::iterator;
        using const_iterator = typename std::unordered_map<SystemId, std::unique_ptr<System>>::const_iterator;

        SystemOwner()
        {}

        template <typename... Args>
        System* CreateNew(SystemId system_id, Args&&... args)
        {
            const std::string_view type_name = typeid(System).name();
            if (Owns(system_id))
            {
                BRR_LogError("System {} (ID: {}) is already owned by this SystemOwner.", type_name, system_id);
                return nullptr;
            }

            auto result = m_systems_map.emplace(system_id, std::forward<Args>(args)...);
            if (!result.second)
            {
                BRR_LogError("Failed to create new {} system. Returning nullptr.", type_name);
                return nullptr;
            }
            auto& system_iter = result.first;
            return system_iter->second.get();
        }

        System* GetSystem(SystemId system_id)
        {
            auto system_iter = m_systems_map.find(system_id);
            if (system_iter == m_systems_map.end())
            {
                return nullptr;
            }
            return system_iter->second.get();
        }

        void Erase(SystemId system_id) noexcept
        {
            m_systems_map.erase(system_id);
        }

        void Clear() noexcept
        {
            m_systems_map.clear();
        }

        bool Owns(SystemId system_id) { return m_systems_map.contains(system_id); }

        iterator Find(SystemId system_id) { return m_systems_map.find(system_id); }
        const_iterator Find(SystemId system_id) const { return m_systems_map.find(system_id); }

        iterator begin() noexcept { return m_systems_map.begin(); }
        iterator end() noexcept { return m_systems_map.end(); }

        const_iterator begin() const noexcept { return m_systems_map.begin(); }
        const_iterator end() const noexcept { return m_systems_map.end(); }

        const_iterator cbegin() const noexcept { return m_systems_map.cbegin(); }
        const_iterator cend() const noexcept { return m_systems_map.cend(); }

    private:

        std::unordered_map<SystemId, std::unique_ptr<System>> m_systems_map;
    };

    struct SystemsStorage
    {
        static SystemOwner<WindowRenderer>& GetWindowRendererStorage();
        static SystemOwner<SceneRenderer>& GetSceneRendererStorage();
    };
}

#endif
