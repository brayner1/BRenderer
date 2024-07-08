#ifndef BRR_RESOURCEMANAGER_H
#define BRR_RESOURCEMANAGER_H

#include <Core/UUID.h>
#include <Core/Ref.h>
#include <Core/Assets/Asset.h>

#include <mutex>
#include <string>
#include <unordered_map>

namespace brr
{
	class AssetManager
	{
	public:
        static bool HasAsset(const std::string& asset_path);
        static bool HasAsset(UUID asset_uuid);

        static Ref<Asset> GetAsset(const std::string& asset_path);
		static Ref<Asset> GetAsset(UUID asset_uuid);

	private:
		friend class Asset;

		static void RegisterAsset(UUID asset_uuid, Asset* asset);
		static void RegisterAsset(std::string asset_path, Asset* asset);

        static void UnregisterAsset(const std::string& asset_path);
		static void UnregisterAsset(UUID asset_uuid);

		static std::mutex s_mutex;
        static std::unordered_map<UUID, Asset*> s_asset_uuid_map;
		static std::unordered_map<std::string, Asset*> s_asset_path_map;
	};

}

#endif