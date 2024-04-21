#ifndef BRR_SCENEOBJECTSIDS_H
#define BRR_SCENEOBJECTSIDS_H

namespace brr::render
{
    enum class CameraId : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class EntityId : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class SurfaceId : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class LightId : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class ViewportId : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };
}

#endif