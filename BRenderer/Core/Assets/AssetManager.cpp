#include "AssetManager.h"

#include <Core/LogSystem.h>

namespace brr
{
    std::mutex AssetManager::s_mutex {};
    std::unordered_map<UUID, Asset*> AssetManager::s_asset_uuid_map {};
	std::unordered_map<std::string, Asset*> AssetManager::s_asset_path_map {};

    bool AssetManager::HasAsset(const std::string& asset_path)
    {
        std::lock_guard lock(s_mutex);
        return s_asset_path_map.contains(asset_path);
    }

    bool AssetManager::HasAsset(UUID asset_uuid)
    {
        std::lock_guard lock(s_mutex);
        return s_asset_uuid_map.contains(asset_uuid);
    }

    Ref<Asset> AssetManager::GetAsset(const std::string& asset_path)
    {
        std::lock_guard lock(s_mutex);
        auto iter = s_asset_path_map.find(asset_path);
        if (iter == s_asset_path_map.end())
        {
            return Ref<Asset>();
        }

        return Ref<Asset>(iter->second);
    }

    Ref<Asset> AssetManager::GetAsset(UUID asset_uuid)
    {
        std::lock_guard lock(s_mutex);
        auto iter = s_asset_uuid_map.find(asset_uuid);
        if (iter == s_asset_uuid_map.end())
        {
            return Ref<Asset>();
        }

        return Ref<Asset>(iter->second);
    }

    void AssetManager::RegisterAsset(UUID asset_uuid,
                                           Asset* asset)
    {
        std::lock_guard lock(s_mutex);
        auto iter = s_asset_uuid_map.find(asset_uuid);
        if (iter != s_asset_uuid_map.end())
        {
            BRR_LogWarn("Asset with UUID: '{}' is already registered.\nRegistered Asset: {:#x}\nInput Asset: {:#x}", asset_uuid, (size_t)iter->second, (size_t)asset);
            return;
        }

        s_asset_uuid_map.emplace(asset_uuid, asset);
    }

    void AssetManager::RegisterAsset(std::string asset_path,
                                           Asset* asset)
    {
        std::lock_guard lock(s_mutex);
        auto iter = s_asset_path_map.find(asset_path);
        if (iter != s_asset_path_map.end())
        {
            BRR_LogWarn("Asset with path: '{}' is already registered.\nRegistered Asset: {:#x}\nInput Asset: {:#x}", asset_path, (size_t)iter->second, (size_t)asset);
            return;
        }

        s_asset_path_map.emplace(asset_path, asset);
    }

    void AssetManager::UnregisterAsset(const std::string& asset_path)
    {
        std::lock_guard lock(s_mutex);
        s_asset_path_map.erase(asset_path);
    }

    void AssetManager::UnregisterAsset(UUID asset_uuid)
    {
        std::lock_guard lock(s_mutex);
        s_asset_uuid_map.erase(asset_uuid);
    }

    
}
