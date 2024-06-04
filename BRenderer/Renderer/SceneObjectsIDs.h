#ifndef BRR_SCENEOBJECTSIDS_H
#define BRR_SCENEOBJECTSIDS_H

namespace brr::render
{
    enum class CameraID : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class EntityID : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class SurfaceID : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class LightID : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class ViewportID : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };
}

#endif