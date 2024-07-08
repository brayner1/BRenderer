#include "Asset.h"

#include <Core/Assets/AssetManager.h>

namespace brr
{
    Asset::Asset()
    : Asset(UUID())
    {}

    Asset::Asset(UUID uuid)
    : m_asset_uuid(uuid)
    {
        AssetManager::RegisterAsset(m_asset_uuid, this);
    }

    Asset::~Asset()
    {
        AssetManager::UnregisterAsset(m_asset_uuid);
        if (!m_asset_path.empty())
        {
            AssetManager::UnregisterAsset(m_asset_path);
        }
    }

    void Asset::SetPath(std::string resource_path, bool replace_previous_path)
    {
        if (!replace_previous_path && !m_asset_path.empty())
        {
            return;
        }

        if (!m_asset_path.empty())
        {
            AssetManager::UnregisterAsset(m_asset_path);
        }

        m_asset_path = std::move(resource_path);
        AssetManager::RegisterAsset(m_asset_path, this);
    }
}
