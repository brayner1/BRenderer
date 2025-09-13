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

    Asset::Asset(std::string asset_path,
                 UUID asset_uuid)
        : m_asset_path(std::move(asset_path)),
          m_asset_uuid(asset_uuid)
    {
        AssetManager::RegisterAsset(m_asset_uuid, this);
        if (!m_asset_path.empty())
        {
            AssetManager::RegisterAsset(m_asset_path, this);
        }
    }

    Asset::~Asset()
    {
        AssetManager::UnregisterAsset(m_asset_uuid);
        if (!m_asset_path.empty())
        {
            AssetManager::UnregisterAsset(m_asset_path);
        }
    }
}
