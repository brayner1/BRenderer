#ifndef BRR_RESOURCE_H
#define BRR_RESOURCE_H

#include <Core/RefCounted.h>
#include <Core/UUID.h>

#include <string>

namespace brr
{
    class Asset : public RefCounted
    {
    public:

        Asset();

        Asset(UUID uuid);
        Asset(std::string asset_path, UUID asset_uuid = UUID());

        virtual ~Asset();

        UUID GetUUID() const { return m_asset_uuid; }

        [[nodiscard]] const std::string& GetPath() const { return m_asset_path; }

    private:

        UUID m_asset_uuid;
        std::string m_asset_path;
    };
}

#endif