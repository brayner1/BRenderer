#ifndef BRR_RENDERDEFS_H
#define BRR_RENDERDEFS_H
#include <cstdint>

namespace brr::render
{
    static constexpr int FRAME_LAG = 2;
    static constexpr uint32_t STAGING_BLOCK_SIZE_KB = 256;
    static constexpr uint32_t STAGING_BUFFER_MAX_SIZE_MB = 128;
    static constexpr uint32_t IMAGE_TRANSFER_BLOCK_SIZE = 64;
}

#endif