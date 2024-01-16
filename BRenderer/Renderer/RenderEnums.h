#ifndef BRR_RENDERENUMS_H
#define BRR_RENDERENUMS_H

namespace brr::render
{
    //
    enum BufferUsage : int
    {
        TransferSrc                             = 1 << 0,
        TransferDst                             = 1 << 1,
        UniformTexelBuffer                      = 1 << 2,
        StorageTexelBuffer                      = 1 << 3,
        UniformBuffer                           = 1 << 4,
        StorageBuffer                           = 1 << 5,
        //IndexBuffer                             = 1 << 6,
        //VertexBuffer                            = 1 << 7,
        //IndirectBuffer                          = 1 << 8,
        //ShaderDeviceAddress                     = 1 << 9,
        //VideoDecodeSrc                          = 1 << 10,
        //VideoDecodeDst                          = 1 << 11,
        //TransformFeedbackBuffer                 = 1 << 12,
        //TransformFeedbackCounterBuffer          = 1 << 13,
        //ConditionalRendering                    = 1 << 14,
        //AccelerationStructureBuildInputReadOnly = 1 << 15,
        //AccelerationStructureStorage            = 1 << 16,
        //ShaderBindingTable                      = 1 << 17,
        //RayTracingNV                            = 1 << 18
    };

    //
    enum ImageUsage : int
    {
        TransferSrcImage			= (1 << 0),
        TransferDstImage			= (1 << 1),
        SampledImage				= (1 << 2),
        StorageImage				= (1 << 3),
        ColorAttachmentImage		= (1 << 4),
        DepthStencilAttachmentImage = (1 << 5),
        TransientAttachmentImage	= (1 << 6),
        InputAttachmentImage		= (1 << 7)
    };

    //
    enum class DataFormat
    {
        Undefined,

        // 8-Bit formats
        R8_UNorm,
        R8_SNorm,
        R8_UScaled,
        R8_SScaled,
        R8_UInt,
        R8_SInt,

        // 16-Bit formats
        R16_UNorm,
        R16_SNorm,
        R16_UScaled,
        R16_SScaled,
        R16_UInt,
        R16_SInt,
        R16_Float,
        R8G8_UNorm,
        R8G8_SNorm,
        R8G8_UScaled,
        R8G8_SScaled,
        R8G8_UInt,
        R8G8_SInt,
        R8G8_SRGB,

        // 24-Bit formats
        R8G8B8_UNorm,
        R8G8B8_SNorm,
        R8G8B8_UScaled,
        R8G8B8_SScaled,
        R8G8B8_UInt,
        R8G8B8_SInt,
        R8G8B8_SRGB,
        B8G8R8_UNorm,
        B8G8R8_SNorm,
        B8G8R8_UScaled,
        B8G8R8_SScaled,
        B8G8R8_UInt,
        B8G8R8_SInt,
        B8G8R8_SRGB,

        // 32-Bit formats
        R32_UInt,
        R32_SInt,
        R32_Float,
        R16G16_UNorm,
        R16G16_SNorm,
        R16G16_UScaled,
        R16G16_SScaled,
        R16G16_UInt,
        R16G16_SInt,
        R16G16_Float,
        R8G8B8A8_UNorm,
        R8G8B8A8_SNorm,
        R8G8B8A8_UScaled,
        R8G8B8A8_SScaled,
        R8G8B8A8_UInt,
        R8G8B8A8_SInt,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNorm,
        B8G8R8A8_SNorm,
        B8G8R8A8_UScaled,
        B8G8R8A8_SScaled,
        B8G8R8A8_UInt,
        B8G8R8A8_SInt,
        B8G8R8A8_SRGB,

        // 48-Bit formats
        R16G16B16_UNorm,
        R16G16B16_SNorm,
        R16G16B16_UScaled,
        R16G16B16_SScaled,
        R16G16B16_UInt,
        R16G16B16_SInt,
        R16G16B16_Float,

        // 64-Bit formats
        R64_UInt,
        R64_SInt,
        R64_Float,
        R32G32_UInt,
        R32G32_SInt,
        R32G32_Float,
        R16G16B16A16_UNorm,
        R16G16B16A16_SNorm,
        R16G16B16A16_UScaled,
        R16G16B16A16_SScaled,
        R16G16B16A16_UInt,
        R16G16B16A16_SInt,
        R16G16B16A16_Float,

        // 96-Bit formats
        R32G32B32_UInt,
        R32G32B32_SInt,
        R32G32B32_Float,

        // 128-Bit formats
        R64G64_UInt,
        R64G64_SInt,
        R64G64_Float,
        R32G32B32A32_UInt,
        R32G32B32A32_SInt,
        R32G32B32A32_Float,

        // 192-Bit formats
        R64G64B64_UInt,
        R64G64B64_SInt,
        R64G64B64_Float,

        // 256-Bit formats
        R64G64B64A64_UInt,
        R64G64B64A64_SInt,
        R64G64B64A64_Float,

        // Depth and Stencil formats
        D16_UNorm,
        D32_Float,
        S8_UInt,
        D16_UNorm_S8_UInt,
        D24_UNorm_S8_UInt,
        D32_Float_S8_UInt

    };

    inline BufferUsage operator|(BufferUsage a, BufferUsage b)
    {
        return static_cast<BufferUsage>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline ImageUsage operator|(ImageUsage a, ImageUsage b)
    {
        return static_cast<ImageUsage>(static_cast<int>(a) | static_cast<int>(b));
    }
}

#endif
