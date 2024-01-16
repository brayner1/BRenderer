#include "VulkanRenderDevice.h"
#include "VulkanRenderDevice.h"

#include <Renderer/RenderDefs.h>

#include <Visualization/Window.h>
#include <Core/LogSystem.h>
#include <Files/FilesUtils.h>
#include <Geometry/Geometry.h>

#include <filesystem>
#include <iostream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
static vk::DynamicLoader VulkanDynamicLoader;

namespace brr::render
{
    static vk::BufferUsageFlags VkBufferUsageFromDeviceBufferUsage(BufferUsage buffer_usage)
    {
        vk::BufferUsageFlags output {};
        if (buffer_usage & BufferUsage::TransferSrc)
        {
            output |= vk::BufferUsageFlagBits::eTransferSrc;
        }
        if (buffer_usage & BufferUsage::TransferDst)
        {
            output |= vk::BufferUsageFlagBits::eTransferDst;
        }
        if (buffer_usage & BufferUsage::UniformTexelBuffer)
        {
            output |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
        }
        if (buffer_usage & BufferUsage::StorageTexelBuffer)
        {
            output |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
        }
        if (buffer_usage & BufferUsage::UniformBuffer)
        {
            output |= vk::BufferUsageFlagBits::eUniformBuffer;
        }
        if (buffer_usage & BufferUsage::StorageBuffer)
        {
            output |= vk::BufferUsageFlagBits::eStorageBuffer;
        }
        //if (buffer_usage & BufferUsage::IndexBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eIndexBuffer;
        //}
        //if (buffer_usage & BufferUsage::VertexBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eVertexBuffer;
        //}
        //if (buffer_usage & BufferUsage::IndirectBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eIndirectBuffer;
        //}
        //if (buffer_usage & BufferUsage::ShaderDeviceAddress)
        //{
        //	output |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
        //}
        //if (buffer_usage & BufferUsage::VideoDecodeSrc)
        //{
        //	output |= vk::BufferUsageFlagBits::eVideoDecodeSrcKHR;
        //}
        //if (buffer_usage & BufferUsage::VideoDecodeDst)
        //{
        //	output |= vk::BufferUsageFlagBits::eVideoDecodeDstKHR;
        //}
        //if (buffer_usage & BufferUsage::TransformFeedbackBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT;
        //}
        //if (buffer_usage & BufferUsage::TransformFeedbackCounterBuffer)
        //{
        //	output |= vk::BufferUsageFlagBits::eTransformFeedbackCounterBufferEXT;
        //}
        //if (buffer_usage & BufferUsage::ConditionalRendering)
        //{
        //	output |= vk::BufferUsageFlagBits::eConditionalRenderingEXT;
        //}
        //if (buffer_usage & BufferUsage::AccelerationStructureBuildInputReadOnly)
        //{
        //	output |= vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
        //}
        //if (buffer_usage & BufferUsage::AccelerationStructureStorage)
        //{
        //	output |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
        //}
        //if (buffer_usage & BufferUsage::ShaderBindingTable)
        //{
        //	output |= vk::BufferUsageFlagBits::eShaderBindingTableKHR;
        //}
        //if (buffer_usage & BufferUsage::RayTracingNV)
        //{
        //	output |= vk::BufferUsageFlagBits::eRayTracingNV;
        //}
        return output;
    }

    static vk::ImageUsageFlags VkImageUsageFromDeviceImageUsage(ImageUsage image_usage)
    {
        vk::ImageUsageFlags result = {};
        if (image_usage & ImageUsage::TransferSrcImage)
        {
            result |= vk::ImageUsageFlagBits::eTransferSrc;
        }
        if (image_usage & ImageUsage::TransferDstImage)
        {
            result |= vk::ImageUsageFlagBits::eTransferDst;
        }
        if (image_usage & ImageUsage::SampledImage)
        {
            result |= vk::ImageUsageFlagBits::eSampled;
        }
        if (image_usage & ImageUsage::StorageImage)
        {
            result |= vk::ImageUsageFlagBits::eStorage;
        }
        if (image_usage & ImageUsage::ColorAttachmentImage)
        {
            result |= vk::ImageUsageFlagBits::eColorAttachment;
        }
        if (image_usage & ImageUsage::DepthStencilAttachmentImage)
        {
            result |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        }
        if (image_usage & ImageUsage::TransientAttachmentImage)
        {
            result |= vk::ImageUsageFlagBits::eTransientAttachment;
        }
        if (image_usage & ImageUsage::InputAttachmentImage)
        {
            result |= vk::ImageUsageFlagBits::eInputAttachment;
        }
        return result;
    }

    static DataFormat DataFormatFromVkFormat(vk::Format vk_format)
    {
        switch (vk_format)
        {
        case vk::Format::eR8Unorm:
            return DataFormat::R8_UNorm;
        case vk::Format::eR8Snorm:
            return DataFormat::R8_SNorm;
        case vk::Format::eR8Uscaled:
            return DataFormat::R8_UScaled;
        case vk::Format::eR8Sscaled:
            return DataFormat::R8_SScaled;
        case vk::Format::eR8Uint:
            return DataFormat::R8_UInt;
        case vk::Format::eR8Sint:
            return DataFormat::R8_SInt;
        case vk::Format::eR16Unorm:
            return DataFormat::R16_UNorm;
        case vk::Format::eR16Snorm:
            return DataFormat::R16_SNorm;
        case vk::Format::eR16Uscaled:
            return DataFormat::R16_UScaled;
        case vk::Format::eR16Sscaled:
            return DataFormat::R16_SScaled;
        case vk::Format::eR16Uint:
            return DataFormat::R16_UInt;
        case vk::Format::eR16Sint:
            return DataFormat::R16_SInt;
        case vk::Format::eR16Sfloat:
            return DataFormat::R16_Float;
        case vk::Format::eR8G8Unorm:
            return DataFormat::R8G8_UNorm;
        case vk::Format::eR8G8Snorm:
            return DataFormat::R8G8_SNorm;
        case vk::Format::eR8G8Uscaled:
            return DataFormat::R8G8_UScaled;
        case vk::Format::eR8G8Sscaled:
            return DataFormat::R8G8_SScaled;
        case vk::Format::eR8G8Uint:
            return DataFormat::R8G8_UInt;
        case vk::Format::eR8G8Sint:
            return DataFormat::R8G8_SInt;
        case vk::Format::eR8G8Srgb:
            return DataFormat::R8G8_SRGB;
        case vk::Format::eR8G8B8Unorm:
            return DataFormat::R8G8B8_UNorm;
        case vk::Format::eR8G8B8Snorm:
            return DataFormat::R8G8B8_SNorm;
        case vk::Format::eR8G8B8Uscaled:
            return DataFormat::R8G8B8_UScaled;
        case vk::Format::eR8G8B8Sscaled:
            return DataFormat::R8G8B8_SScaled;
        case vk::Format::eR8G8B8Uint:
            return DataFormat::R8G8B8_UInt;
        case vk::Format::eR8G8B8Sint:
            return DataFormat::R8G8B8_SInt;
        case vk::Format::eR8G8B8Srgb:
            return DataFormat::R8G8B8_SRGB;
        case vk::Format::eB8G8R8Unorm:
            return DataFormat::B8G8R8_UNorm;
        case vk::Format::eB8G8R8Snorm:
            return DataFormat::B8G8R8_SNorm;
        case vk::Format::eB8G8R8Uscaled:
            return DataFormat::B8G8R8_UScaled;
        case vk::Format::eB8G8R8Sscaled:
            return DataFormat::B8G8R8_SScaled;
        case vk::Format::eB8G8R8Uint:
            return DataFormat::B8G8R8_UInt;
        case vk::Format::eB8G8R8Sint:
            return DataFormat::B8G8R8_SInt;
        case vk::Format::eB8G8R8Srgb:
            return DataFormat::B8G8R8_SRGB;
        case vk::Format::eR32Uint:
            return DataFormat::R32_UInt;
        case vk::Format::eR32Sint:
            return DataFormat::R32_SInt;
        case vk::Format::eR32Sfloat:
            return DataFormat::R32_Float;
        case vk::Format::eR16G16Unorm:
            return DataFormat::R16G16_UNorm;
        case vk::Format::eR16G16Snorm:
            return DataFormat::R16G16_SNorm;
        case vk::Format::eR16G16Uscaled:
            return DataFormat::R16G16_UScaled;
        case vk::Format::eR16G16Sscaled:
            return DataFormat::R16G16_SScaled;
        case vk::Format::eR16G16Uint:
            return DataFormat::R16G16_UInt;
        case vk::Format::eR16G16Sint:
            return DataFormat::R16G16_SInt;
        case vk::Format::eR16G16Sfloat:
            return DataFormat::R16G16_Float;
        case vk::Format::eR8G8B8A8Unorm:
            return DataFormat::R8G8B8A8_UNorm;
        case vk::Format::eR8G8B8A8Snorm:
            return DataFormat::R8G8B8A8_SNorm;
        case vk::Format::eR8G8B8A8Uscaled:
            return DataFormat::R8G8B8A8_UScaled;
        case vk::Format::eR8G8B8A8Sscaled:
            return DataFormat::R8G8B8A8_SScaled;
        case vk::Format::eR8G8B8A8Uint:
            return DataFormat::R8G8B8A8_UInt;
        case vk::Format::eR8G8B8A8Sint:
            return DataFormat::R8G8B8A8_SInt;
        case vk::Format::eR8G8B8A8Srgb:
            return DataFormat::R8G8B8A8_SRGB;
        case vk::Format::eB8G8R8A8Unorm:
            return DataFormat::B8G8R8A8_UNorm;
        case vk::Format::eB8G8R8A8Snorm:
            return DataFormat::B8G8R8A8_SNorm;
        case vk::Format::eB8G8R8A8Uscaled:
            return DataFormat::B8G8R8A8_UScaled;
        case vk::Format::eB8G8R8A8Sscaled:
            return DataFormat::B8G8R8A8_SScaled;
        case vk::Format::eB8G8R8A8Uint:
            return DataFormat::B8G8R8A8_UInt;
        case vk::Format::eB8G8R8A8Sint:
            return DataFormat::B8G8R8A8_SInt;
        case vk::Format::eB8G8R8A8Srgb:
            return DataFormat::B8G8R8A8_SRGB;
        case vk::Format::eR16G16B16Unorm:
            return DataFormat::R16G16B16_UNorm;
        case vk::Format::eR16G16B16Snorm:
            return DataFormat::R16G16B16_SNorm;
        case vk::Format::eR16G16B16Uscaled:
            return DataFormat::R16G16B16_UScaled;
        case vk::Format::eR16G16B16Sscaled:
            return DataFormat::R16G16B16_SScaled;
        case vk::Format::eR16G16B16Uint:
            return DataFormat::R16G16B16_UInt;
        case vk::Format::eR16G16B16Sint:
            return DataFormat::R16G16B16_SInt;
        case vk::Format::eR16G16B16Sfloat:
            return DataFormat::R16G16B16_Float;
        case vk::Format::eR64Uint:
            return DataFormat::R64_UInt;
        case vk::Format::eR64Sint:
            return DataFormat::R64_SInt;
        case vk::Format::eR64Sfloat:
            return DataFormat::R64_Float;
        case vk::Format::eR32G32Uint:
            return DataFormat::R32G32_UInt;
        case vk::Format::eR32G32Sint:
            return DataFormat::R32G32_SInt;
        case vk::Format::eR32G32Sfloat:
            return DataFormat::R32G32_Float;
        case vk::Format::eR16G16B16A16Unorm:
            return DataFormat::R16G16B16A16_UNorm;
        case vk::Format::eR16G16B16A16Snorm:
            return DataFormat::R16G16B16A16_SNorm;
        case vk::Format::eR16G16B16A16Uscaled:
            return DataFormat::R16G16B16A16_UScaled;
        case vk::Format::eR16G16B16A16Sscaled:
            return DataFormat::R16G16B16A16_SScaled;
        case vk::Format::eR16G16B16A16Uint:
            return DataFormat::R16G16B16A16_UInt;
        case vk::Format::eR16G16B16A16Sint:
            return DataFormat::R16G16B16A16_SInt;
        case vk::Format::eR16G16B16A16Sfloat:
            return DataFormat::R16G16B16A16_Float;
        case vk::Format::eR32G32B32Uint:
            return DataFormat::R32G32B32_UInt;
        case vk::Format::eR32G32B32Sint:
            return DataFormat::R32G32B32_SInt;
        case vk::Format::eR32G32B32Sfloat:
            return DataFormat::R32G32B32_Float;
        case vk::Format::eR64G64Uint:
            return DataFormat::R64G64_UInt;
        case vk::Format::eR64G64Sint:
            return DataFormat::R64G64_SInt;
        case vk::Format::eR64G64Sfloat:
            return DataFormat::R64G64_Float;
        case vk::Format::eR32G32B32A32Uint:
            return DataFormat::R32G32B32A32_UInt;
        case vk::Format::eR32G32B32A32Sint:
            return DataFormat::R32G32B32A32_SInt;
        case vk::Format::eR32G32B32A32Sfloat:
            return DataFormat::R32G32B32A32_Float;
        case vk::Format::eR64G64B64Uint:
            return DataFormat::R64G64B64_UInt;
        case vk::Format::eR64G64B64Sint:
            return DataFormat::R64G64B64_SInt;
        case vk::Format::eR64G64B64Sfloat:
            return DataFormat::R64G64B64_Float;
        case vk::Format::eR64G64B64A64Uint:
            return DataFormat::R64G64B64A64_UInt;
        case vk::Format::eR64G64B64A64Sint:
            return DataFormat::R64G64B64A64_SInt;
        case vk::Format::eR64G64B64A64Sfloat:
            return DataFormat::R64G64B64A64_Float;
        case vk::Format::eD16Unorm:
            return DataFormat::D16_UNorm;
        case vk::Format::eD32Sfloat:
            return DataFormat::D32_Float;
        case vk::Format::eS8Uint:
            return DataFormat::S8_UInt;
        case vk::Format::eD16UnormS8Uint:
            return DataFormat::D16_UNorm_S8_UInt;
        case vk::Format::eD24UnormS8Uint:
            return DataFormat::D24_UNorm_S8_UInt;
        case vk::Format::eD32SfloatS8Uint:
            return DataFormat::D32_Float_S8_UInt;
        default:
            return DataFormat::Undefined;
        }
    }

    vk::Format VulkanRenderDevice::VkFormatFromDeviceDataFormat(DataFormat data_format)
    {
        switch (data_format)
        {
        case DataFormat::R8_UNorm:
            return vk::Format::eR8Unorm;
        case DataFormat::R8_SNorm:
            return vk::Format::eR8Snorm;
        case DataFormat::R8_UScaled:
            return vk::Format::eR8Uscaled;
        case DataFormat::R8_SScaled:
            return vk::Format::eR8Sscaled;
        case DataFormat::R8_UInt:
            return vk::Format::eR8Uint;
        case DataFormat::R8_SInt:
            return vk::Format::eR8Sint;
        case DataFormat::R16_UNorm:
            return vk::Format::eR16Unorm;
        case DataFormat::R16_SNorm:
            return vk::Format::eR16Snorm;
        case DataFormat::R16_UScaled:
            return vk::Format::eR16Uscaled;
        case DataFormat::R16_SScaled:
            return vk::Format::eR16Sscaled;
        case DataFormat::R16_UInt:
            return vk::Format::eR16Uint;
        case DataFormat::R16_SInt:
            return vk::Format::eR16Sint;
        case DataFormat::R16_Float:
            return vk::Format::eR16Sfloat;
        case DataFormat::R8G8_UNorm:
            return vk::Format::eR8G8Unorm;
        case DataFormat::R8G8_SNorm:
            return vk::Format::eR8G8Snorm;
        case DataFormat::R8G8_UScaled:
            return vk::Format::eR8G8Uscaled;
        case DataFormat::R8G8_SScaled:
            return vk::Format::eR8G8Sscaled;
        case DataFormat::R8G8_UInt:
            return vk::Format::eR8G8Uint;
        case DataFormat::R8G8_SInt:
            return vk::Format::eR8G8Sint;
        case DataFormat::R8G8_SRGB:
            return vk::Format::eR8G8Srgb;
        case DataFormat::R8G8B8_UNorm:
            return vk::Format::eR8G8B8Unorm;
        case DataFormat::R8G8B8_SNorm:
            return vk::Format::eR8G8B8Snorm;
        case DataFormat::R8G8B8_UScaled:
            return vk::Format::eR8G8B8Uscaled;
        case DataFormat::R8G8B8_SScaled:
            return vk::Format::eR8G8B8Sscaled;
        case DataFormat::R8G8B8_UInt:
            return vk::Format::eR8G8B8Uint;
        case DataFormat::R8G8B8_SInt:
            return vk::Format::eR8G8B8Sint;
        case DataFormat::R8G8B8_SRGB:
            return vk::Format::eR8G8B8Srgb;
        case DataFormat::B8G8R8_UNorm:
            return vk::Format::eB8G8R8Unorm;
        case DataFormat::B8G8R8_SNorm:
            return vk::Format::eB8G8R8Snorm;
        case DataFormat::B8G8R8_UScaled:
            return vk::Format::eB8G8R8Uscaled;
        case DataFormat::B8G8R8_SScaled:
            return vk::Format::eB8G8R8Sscaled;
        case DataFormat::B8G8R8_UInt:
            return vk::Format::eB8G8R8Uint;
        case DataFormat::B8G8R8_SInt:
            return vk::Format::eB8G8R8Sint;
        case DataFormat::B8G8R8_SRGB:
            return vk::Format::eB8G8R8Srgb;
        case DataFormat::R32_UInt:
            return vk::Format::eR32Uint;
        case DataFormat::R32_SInt:
            return vk::Format::eR32Sint;
        case DataFormat::R32_Float:
            return vk::Format::eR32Sfloat;
        case DataFormat::R16G16_UNorm:
            return vk::Format::eR16G16Unorm;
        case DataFormat::R16G16_SNorm:
            return vk::Format::eR16G16Snorm;
        case DataFormat::R16G16_UScaled:
            return vk::Format::eR16G16Uscaled;
        case DataFormat::R16G16_SScaled:
            return vk::Format::eR16G16Sscaled;
        case DataFormat::R16G16_UInt:
            return vk::Format::eR16G16Uint;
        case DataFormat::R16G16_SInt:
            return vk::Format::eR16G16Sint;
        case DataFormat::R16G16_Float:
            return vk::Format::eR16G16Sfloat;
        case DataFormat::R8G8B8A8_UNorm:
            return vk::Format::eR8G8B8A8Unorm;
        case DataFormat::R8G8B8A8_SNorm:
            return vk::Format::eR8G8B8A8Snorm;
        case DataFormat::R8G8B8A8_UScaled:
            return vk::Format::eR8G8B8A8Uscaled;
        case DataFormat::R8G8B8A8_SScaled:
            return vk::Format::eR8G8B8A8Sscaled;
        case DataFormat::R8G8B8A8_UInt:
            return vk::Format::eR8G8B8A8Uint;
        case DataFormat::R8G8B8A8_SInt:
            return vk::Format::eR8G8B8A8Sint;
        case DataFormat::R8G8B8A8_SRGB:
            return vk::Format::eR8G8B8A8Srgb;
        case DataFormat::B8G8R8A8_UNorm:
            return vk::Format::eB8G8R8A8Unorm;
        case DataFormat::B8G8R8A8_SNorm:
            return vk::Format::eB8G8R8A8Snorm;
        case DataFormat::B8G8R8A8_UScaled:
            return vk::Format::eB8G8R8A8Uscaled;
        case DataFormat::B8G8R8A8_SScaled:
            return vk::Format::eB8G8R8A8Sscaled;
        case DataFormat::B8G8R8A8_UInt:
            return vk::Format::eB8G8R8A8Uint;
        case DataFormat::B8G8R8A8_SInt:
            return vk::Format::eB8G8R8A8Sint;
        case DataFormat::B8G8R8A8_SRGB:
            return vk::Format::eB8G8R8A8Srgb;
        case DataFormat::R16G16B16_UNorm:
            return vk::Format::eR16G16B16Unorm;
        case DataFormat::R16G16B16_SNorm:
            return vk::Format::eR16G16B16Snorm;
        case DataFormat::R16G16B16_UScaled:
            return vk::Format::eR16G16B16Uscaled;
        case DataFormat::R16G16B16_SScaled:
            return vk::Format::eR16G16B16Sscaled;
        case DataFormat::R16G16B16_UInt:
            return vk::Format::eR16G16B16Uint;
        case DataFormat::R16G16B16_SInt:
            return vk::Format::eR16G16B16Sint;
        case DataFormat::R16G16B16_Float:
            return vk::Format::eR16G16B16Sfloat;
        case DataFormat::R64_UInt:
            return vk::Format::eR64Uint;
        case DataFormat::R64_SInt:
            return vk::Format::eR64Sint;
        case DataFormat::R64_Float:
            return vk::Format::eR64Sfloat;
        case DataFormat::R32G32_UInt:
            return vk::Format::eR32G32Uint;
        case DataFormat::R32G32_SInt:
            return vk::Format::eR32G32Sint;
        case DataFormat::R32G32_Float:
            return vk::Format::eR32G32Sfloat;
        case DataFormat::R16G16B16A16_UNorm:
            return vk::Format::eR16G16B16A16Unorm;;
        case DataFormat::R16G16B16A16_SNorm:
            return vk::Format::eR16G16B16A16Snorm;
        case DataFormat::R16G16B16A16_UScaled:
            return vk::Format::eR16G16B16A16Uscaled;
        case DataFormat::R16G16B16A16_SScaled:
            return vk::Format::eR16G16B16A16Sscaled;
        case DataFormat::R16G16B16A16_UInt:
            return vk::Format::eR16G16B16A16Uint;
        case DataFormat::R16G16B16A16_SInt:
            return vk::Format::eR16G16B16A16Sint;
        case DataFormat::R16G16B16A16_Float:
            return vk::Format::eR16G16B16A16Sfloat;
        case DataFormat::R32G32B32_UInt:
            return vk::Format::eR32G32B32Uint;
        case DataFormat::R32G32B32_SInt:
            return vk::Format::eR32G32B32Sint;
        case DataFormat::R32G32B32_Float:
            return vk::Format::eR32G32B32Sfloat;
        case DataFormat::R64G64_UInt:
            return vk::Format::eR64G64Uint;
        case DataFormat::R64G64_SInt:
            return vk::Format::eR64G64Sint;
        case DataFormat::R64G64_Float:
            return vk::Format::eR64G64Sfloat;
        case DataFormat::R32G32B32A32_UInt:
            return vk::Format::eR32G32B32A32Uint;
        case DataFormat::R32G32B32A32_SInt:
            return vk::Format::eR32G32B32A32Sint;
        case DataFormat::R32G32B32A32_Float:
            return vk::Format::eR32G32B32A32Sfloat;
        case DataFormat::R64G64B64_UInt:
            return vk::Format::eR64G64B64Uint;
        case DataFormat::R64G64B64_SInt:
            return vk::Format::eR64G64B64Sint;
        case DataFormat::R64G64B64_Float:
            return vk::Format::eR64G64B64Sfloat;
        case DataFormat::R64G64B64A64_UInt:
            return vk::Format::eR64G64B64A64Uint;
        case DataFormat::R64G64B64A64_SInt:
            return vk::Format::eR64G64B64A64Sint;
        case DataFormat::R64G64B64A64_Float:
            return vk::Format::eR64G64B64A64Sfloat;
        case DataFormat::D16_UNorm:
            return vk::Format::eD16Unorm;
        case DataFormat::D32_Float:
            return vk::Format::eD32Sfloat;
        case DataFormat::S8_UInt:
            return vk::Format::eS8Uint;
        case DataFormat::D16_UNorm_S8_UInt:
            return vk::Format::eD16UnormS8Uint;
        case DataFormat::D24_UNorm_S8_UInt:
            return vk::Format::eD24UnormS8Uint;
        case DataFormat::D32_Float_S8_UInt:
            return vk::Format::eD32SfloatS8Uint;
        case DataFormat::Undefined:
        default:
            return vk::Format::eUndefined;
        }
    }

    VmaMemoryUsage VmaMemoryUsageFromDeviceMemoryUsage(VulkanRenderDevice::MemoryUsage memory_usage)
    {
        switch (memory_usage)
        {
        case VulkanRenderDevice::AUTO:
            return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
        case VulkanRenderDevice::AUTO_PREFER_DEVICE:
            return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        case VulkanRenderDevice::AUTO_PREFER_HOST:
            return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        default:
            return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
        }
    }

    static vk::ShaderModule Create_ShaderModule(VulkanRenderDevice* device, std::vector<char>& code)
    {
        vk::ShaderModuleCreateInfo shader_module_info{};
        shader_module_info
            .setCodeSize(code.size())
            .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

        auto createShaderModuleResult = device->Get_VkDevice().createShaderModule(shader_module_info);
        if (createShaderModuleResult.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create ShaderModule! Result code: {}.", vk::to_string(createShaderModuleResult.result).c_str());
            return VK_NULL_HANDLE;
        }
        return createShaderModuleResult.value;
    }

    std::unique_ptr<VulkanRenderDevice> VulkanRenderDevice::device_instance {};

    void VulkanRenderDevice::CreateRenderDevice(vis::Window* window)
    {
        assert(!device_instance && "VulkanRenderDevice is already created. You can only create one.");
        device_instance.reset(new VulkanRenderDevice(window));
    }

    void VulkanRenderDevice::DestroyRenderDevice()
    {
        device_instance.reset();
    }

    VulkanRenderDevice* VulkanRenderDevice::GetSingleton()
    {
        assert(device_instance && "Can't get non-initialized VulkanRenderDevice. Run `VKRD::CreateRenderDevice(Window* window)` before this function.");
        return device_instance.get();
    }

    VulkanRenderDevice::~VulkanRenderDevice()
    {
        BRR_LogDebug("Starting VulkanRenderDevice destruction.");

        BRR_LogTrace("Waiting device idle.");
        WaitIdle();
        BRR_LogTrace("Device idle. Starting destroy process");

        for (size_t idx = 0; idx < FRAME_LAG; ++idx)
        {
            Free_FramePendingResources(m_frames[idx]);
            m_device.destroySemaphore(m_frames[idx].render_finished_semaphore);
            m_device.destroySemaphore(m_frames[idx].transfer_finished_semaphore);
        }
        BRR_LogTrace("Destroyed frame sync semaphores.");

        m_staging_allocator.DestroyAllocator();
        BRR_LogTrace("Destroyed staging allocator.");

        vmaDestroyAllocator(m_vma_allocator);
        BRR_LogTrace("Destroyed VMA allocator.");

        m_device.destroySampler(m_texture2DSampler);
        m_texture2DSampler = VK_NULL_HANDLE;

        m_descriptor_layout_cache.reset();
        BRR_LogTrace("Destroyed descriptor layout cache.");
        m_descriptor_allocator.reset();
        BRR_LogTrace("Destroyed descriptor allocator.");

        if (m_graphics_command_pool)
        {
            m_device.destroyCommandPool(m_graphics_command_pool);
            m_graphics_command_pool = VK_NULL_HANDLE;
            BRR_LogTrace("Destroyed graphics command pool.");
        }
        if (m_present_command_pool)
        {
            m_device.destroyCommandPool(m_present_command_pool);
            m_present_command_pool = VK_NULL_HANDLE;
            BRR_LogTrace("Destroyed present command pool.");
        }
        if (m_transfer_command_pool)
        {
            m_device.destroyCommandPool(m_transfer_command_pool);
            m_transfer_command_pool = VK_NULL_HANDLE;
            BRR_LogTrace("Destroyed transfer command pool.");
        }

        m_device.destroy();
        BRR_LogTrace("Destroyed Vulkan device.");

        BRR_LogInfo("VulkanRenderDevice destroyed.");
    }

    uint32_t VulkanRenderDevice::BeginFrame()
    {
        Frame& current_frame = m_frames[m_current_buffer];

        Free_FramePendingResources(current_frame);

        if (!current_frame.graphics_cmd_buffer_begin)
        {
            vk::Result graph_begin_result = BeginGraphicsCommandBuffer(current_frame.graphics_cmd_buffer);
            current_frame.graphics_cmd_buffer_begin = true;
        }

        if (!current_frame.transfer_cmd_buffer_begin)
        {
            vk::Result transf_begin_result = BeginTransferCommandBuffer(current_frame.transfer_cmd_buffer);
            current_frame.transfer_cmd_buffer_begin = true;
        }

        BRR_LogTrace("Begin frame {}", m_current_frame);

        return m_current_frame;
    }

    vk::Semaphore VulkanRenderDevice::EndFrame(vk::Semaphore wait_semaphore, vk::Fence wait_fence)
    {
        Frame& current_frame = m_frames[m_current_buffer];

        current_frame.graphics_cmd_buffer.end();
        current_frame.transfer_cmd_buffer.end();

        current_frame.graphics_cmd_buffer_begin = false;
        current_frame.transfer_cmd_buffer_begin = false;

        vk::Result transfer_result = SubmitTransferCommandBuffers(1, &current_frame.transfer_cmd_buffer, 0, nullptr, nullptr, 1, &current_frame.transfer_finished_semaphore, nullptr);
        BRR_LogTrace("Transfer command buffer submitted. Buffer: {:#x}. Frame {}. Buffer Index: {}", size_t(VkCommandBuffer((current_frame.transfer_cmd_buffer))), m_current_frame, m_current_buffer);

        std::array<vk::Semaphore, 2> wait_semaphores { wait_semaphore, current_frame.transfer_finished_semaphore };
        std::array<vk::PipelineStageFlags, 2> wait_stages { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eVertexInput };

        vk::Result result = SubmitGraphicsCommandBuffers(1, &current_frame.graphics_cmd_buffer, 2, wait_semaphores.data(), wait_stages.data(), 1, &current_frame.render_finished_semaphore, wait_fence);
        BRR_LogTrace("Graphics command buffer submitted. Buffer: {:#x}. Frame {}. Buffer Index: {}", size_t(VkCommandBuffer(current_frame.graphics_cmd_buffer)), m_current_frame, m_current_buffer);

        BRR_LogTrace("Frame ended. Frame {}. Buffer: {}", m_current_frame, m_current_buffer);

        ++m_current_frame;
        m_current_buffer = (m_current_frame % FRAME_LAG);

        return current_frame.render_finished_semaphore;
    }

    VulkanRenderDevice::VulkanRenderDevice(vis::Window* main_window)
    {
        BRR_LogInfo("Constructing VulkanRenderDevice");
        Init_VkInstance(main_window);
        vk::SurfaceKHR surface = main_window->GetVulkanSurface(m_vulkan_instance);
        Init_PhysDevice(surface);
        Init_Queues_Indices(surface);
        Init_Device();
        Init_Allocator();
        Init_CommandPool();
        Init_Frames();

        m_staging_allocator.Init(this);

        m_descriptor_layout_cache.reset(new DescriptorLayoutCache(m_device));
        m_descriptor_allocator.reset(new DescriptorAllocator(m_device));

        Init_Texture2DSampler();

        BRR_LogInfo("VulkanRenderDevice {:#x} constructed", (size_t)this);
    }

    void VulkanRenderDevice::WaitIdle() const
    {
        BRR_LogTrace("Waiting for device idle.");
        m_device.waitIdle();
    }

    static vk::Result allocateCommandBuffer(vk::Device device, vk::CommandPool cmd_pool, vk::CommandBufferLevel level,
                                            uint32_t cmd_buffer_count, vk::CommandBuffer* out_command_buffers)
    {
        vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
        command_buffer_alloc_info
            .setCommandPool(cmd_pool)
            .setLevel(vk::CommandBufferLevel(level))
            .setCommandBufferCount(cmd_buffer_count);

        auto allocCmdBufferResult = device.allocateCommandBuffers(command_buffer_alloc_info);
        if (allocCmdBufferResult.result == vk::Result::eSuccess)
        {
            std::memcpy(out_command_buffers, allocCmdBufferResult.value.data(), cmd_buffer_count * sizeof(vk::CommandBuffer));
        }
        return allocCmdBufferResult.result;
    }

    vk::CommandBuffer VulkanRenderDevice::GetCurrentGraphicsCommandBuffer()
    {
        Frame& current_frame = m_frames[m_current_buffer];

        return current_frame.graphics_cmd_buffer;
    }

    vk::CommandBuffer VulkanRenderDevice::GetCurrentTransferCommandBuffer()
    {
        Frame& current_frame = m_frames[m_current_buffer];

        return current_frame.transfer_cmd_buffer;
    }

    DescriptorLayoutBuilder VulkanRenderDevice::GetDescriptorLayoutBuilder() const
    {
        return DescriptorLayoutBuilder::MakeDescriptorLayoutBuilder(m_descriptor_layout_cache.get());
    }

    DescriptorSetBuilder<FRAME_LAG> VulkanRenderDevice::GetDescriptorSetBuilder(
        const DescriptorLayout& layout) const
    {
        return DescriptorSetBuilder<FRAME_LAG>::MakeDescriptorSetBuilder(layout, m_descriptor_allocator.get());
    }

    BufferHandle VulkanRenderDevice::CreateBuffer(size_t buffer_size, BufferUsage buffer_usage,
                                                   MemoryUsage memory_usage,
                                                   VmaAllocationCreateFlags buffer_allocation_flags)
    {
        BufferHandle buffer_handle;
        // Create Buffer
        {
            const ResourceHandle resource_handle = m_buffer_alloc.CreateResource();
            buffer_handle.index = resource_handle.index; buffer_handle.validation = resource_handle.validation;
            Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);

            vk::SharingMode sharing_mode = IsDifferentTransferQueue() ? vk::SharingMode::eExclusive : vk::SharingMode::eExclusive;

            const vk::BufferUsageFlags vk_buffer_usage = VkBufferUsageFromDeviceBufferUsage(buffer_usage);

            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info
                .setUsage(vk_buffer_usage)
                .setSharingMode(sharing_mode)
                .setSize(buffer_size);
            if (sharing_mode == vk::SharingMode::eConcurrent)
            {
                std::array<uint32_t, 2> indices{
                    GetQueueFamilyIndices().m_graphicsFamily.value(),
                    GetQueueFamilyIndices().m_transferFamily.value()
                };
                buffer_create_info.setQueueFamilyIndices(indices);
            }

            VmaMemoryUsage vma_memory_usage = VmaMemoryUsageFromDeviceMemoryUsage(memory_usage);

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = vma_memory_usage;
            allocInfo.flags = buffer_allocation_flags;
            allocInfo.requiredFlags = 0;
            allocInfo.preferredFlags = 0;
            allocInfo.memoryTypeBits = 0;
            allocInfo.pool = VK_NULL_HANDLE;
            allocInfo.pUserData = nullptr;
            allocInfo.priority = 1.0;

            VkBuffer new_buffer;
            VmaAllocation allocation;
            VmaAllocationInfo allocation_info;
            const vk::Result createBufferResult = vk::Result(vmaCreateBuffer(m_vma_allocator, reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &allocInfo,
                                                                             &new_buffer, &allocation, &allocation_info));

            if (createBufferResult != vk::Result::eSuccess)
            {
                BRR_LogError("Could not create Buffer! Result code: {}.", vk::to_string(createBufferResult).c_str());
                return BufferHandle();
            }
            buffer->buffer = new_buffer;
            buffer->buffer_allocation = allocation;
            buffer->allocation_info = allocation_info;
            buffer->buffer_size = buffer_size;
            buffer->buffer_usage = vk_buffer_usage;

            BRR_LogDebug("Buffer created. Buffer: {:#x}.", size_t(VkBuffer(buffer->buffer)));
        }

        return buffer_handle;
    }

    bool VulkanRenderDevice::DestroyBuffer(BufferHandle buffer_handle)
    {
        Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);
        if (!buffer)
        {
            return false;
        }

        Frame& current_frame = m_frames[m_current_buffer];
        current_frame.buffer_delete_list.emplace_back(buffer->buffer, buffer->buffer_allocation);

        m_buffer_alloc.DestroyResource(buffer_handle);

        BRR_LogDebug("Buffer destroyed. Buffer: {:#x}.", size_t(VkBuffer(buffer->buffer)));
        return true;
    }

    void* VulkanRenderDevice::MapBuffer(BufferHandle buffer_handle)
    {
        Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);

        if (buffer->mapped)
        {
            return buffer->mapped;
        }

        vk::Result result = (vk::Result)vmaMapMemory(m_vma_allocator, buffer->buffer_allocation, &buffer->mapped);
        if (result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not map buffer memory! Result code: {}.", vk::to_string(result).c_str());
            buffer->mapped = nullptr;
        }

        BRR_LogTrace("Mapped buffer. Buffer: {:#x}. Mapped adress: {:#x}.", size_t(VkBuffer(buffer->buffer)), (size_t)buffer->mapped);

        return buffer->mapped;
    }

    void VulkanRenderDevice::UnmapBuffer(BufferHandle buffer_handle)
    {
        Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);

        if (!buffer->mapped)
        {
            return;
        }

        vmaUnmapMemory(m_vma_allocator, buffer->buffer_allocation);
        buffer->mapped = nullptr;

        BRR_LogTrace("Unmapped buffer. Buffer: {:#x}.", size_t(VkBuffer(buffer->buffer)));
    }

    bool VulkanRenderDevice::UploadBufferData(BufferHandle dst_buffer_handle, void* data, size_t size, uint32_t offset)
    {
        Buffer* dst_buffer = m_buffer_alloc.GetResource(dst_buffer_handle);

        uint32_t written_bytes = 0;
        while (written_bytes != size)
        {
            render::StagingBufferHandle staging_buffer{};
            const uint32_t allocated = m_staging_allocator.AllocateStagingBuffer(m_current_frame, size - written_bytes, &staging_buffer);

            m_staging_allocator.WriteLinearBufferToStaging(staging_buffer, 0, static_cast<char*>(data) + written_bytes, allocated);

            m_staging_allocator.CopyFromStagingToBuffer(staging_buffer, dst_buffer->buffer, allocated, 0, written_bytes);

            written_bytes += allocated;
        }

        return true;
    }

    bool VulkanRenderDevice::CopyBuffer(BufferHandle src_buffer_handle, BufferHandle dst_buffer_handle, size_t size,
                                        uint32_t src_buffer_offset, uint32_t dst_buffer_offset, bool use_transfer_queue)
    {
        Frame& current_frame = m_frames[m_current_buffer];

        vk::CommandBuffer cmd_buffer = use_transfer_queue? current_frame.transfer_cmd_buffer : current_frame.graphics_cmd_buffer;

        Buffer* src_buffer = m_buffer_alloc.GetResource(src_buffer_handle);
        Buffer* dst_buffer = m_buffer_alloc.GetResource(dst_buffer_handle);

        vk::BufferCopy copy_region{};
        copy_region
            .setSrcOffset(src_buffer_offset)
            .setDstOffset(dst_buffer_offset)
            .setSize(size);

        cmd_buffer.copyBuffer(src_buffer->buffer, dst_buffer->buffer, copy_region);

        //TODO: Check if src buffer is on CPU or GPU to decide if uses transfer queue or no.
        if (use_transfer_queue)
        {
            {
                vk::BufferMemoryBarrier2 src_buffer_memory_barrier;
                src_buffer_memory_barrier
                    .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                    .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                    .setSrcQueueFamilyIndex(GetQueueFamilyIndices().m_transferFamily.value())
                    .setDstQueueFamilyIndex(GetQueueFamilyIndices().m_graphicsFamily.value())
                    .setBuffer(dst_buffer->buffer)
                    .setSize(size);

                vk::DependencyInfo dependency_info;
                dependency_info
                    .setBufferMemoryBarriers(src_buffer_memory_barrier);

                current_frame.transfer_cmd_buffer.pipelineBarrier2(dependency_info);
            }

            {
                vk::BufferMemoryBarrier2 dst_buffer_memory_barrier {};
                dst_buffer_memory_barrier
                    .setBuffer(dst_buffer->buffer)
                    .setSize(size)
                    .setSrcQueueFamilyIndex(GetQueueFamilyIndices().m_transferFamily.value())
                    .setDstQueueFamilyIndex(GetQueueFamilyIndices().m_graphicsFamily.value())
                    .setDstStageMask(vk::PipelineStageFlagBits2::eVertexAttributeInput)
                    .setDstAccessMask(vk::AccessFlagBits2::eVertexAttributeRead);

                vk::DependencyInfo dependency_info {};
                dependency_info
                    .setBufferMemoryBarriers(dst_buffer_memory_barrier);

                current_frame.graphics_cmd_buffer.pipelineBarrier2(dependency_info);
            }
        }
        else
        {
            //TODO: Select DstAccessMask and DstStageMask based on buffer usage.
            vk::MemoryBarrier memory_barrier;
            memory_barrier
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead);

            current_frame.graphics_cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                                              vk::PipelineStageFlagBits::eVertexInput,
                                                              vk::DependencyFlags(), 1, &memory_barrier, 0,
                                                              nullptr, 0, nullptr);
        }

        return true;
    }

    bool VulkanRenderDevice::CopyBuffer_Immediate(BufferHandle src_buffer_handle, BufferHandle dst_buffer_handle,
                                                  size_t size,
                                                  uint32_t src_buffer_offset, uint32_t dst_buffer_offset)
    {
        Buffer* src_buffer = m_buffer_alloc.GetResource(src_buffer_handle);
        Buffer* dst_buffer = m_buffer_alloc.GetResource(dst_buffer_handle);

        const vk::CommandPool transfer_cmd_pool = (IsDifferentTransferQueue()) ? m_transfer_command_pool : m_graphics_command_pool;

        vk::CommandBufferAllocateInfo cmd_buffer_alloc_info{};
        cmd_buffer_alloc_info
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandPool(transfer_cmd_pool)
            .setCommandBufferCount(1);

        auto allocCmdBuffersResult = m_device.allocateCommandBuffers(cmd_buffer_alloc_info);
        if (allocCmdBuffersResult.result != vk::Result::eSuccess)
        {
            BRR_LogError("ERROR: Could not allocate CommandBuffer! Result code: {}.", vk::to_string(allocCmdBuffersResult.result).c_str());
            return false;
        }
        vk::CommandBuffer cmd_buffer = allocCmdBuffersResult.value[0];

        vk::CommandBufferBeginInfo cmd_begin_info{};
        cmd_begin_info
            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        cmd_buffer.begin(cmd_begin_info);

        vk::BufferCopy copy_region{};
        copy_region
            .setSrcOffset(src_buffer_offset)
            .setDstOffset(dst_buffer_offset)
            .setSize(size);

        cmd_buffer.copyBuffer(src_buffer->buffer, dst_buffer->buffer, copy_region);

        cmd_buffer.end();

        WaitIdle();

        SubmitTransferCommandBuffers(1, &cmd_buffer, 0, nullptr, nullptr, 0, nullptr, VK_NULL_HANDLE);

        GetTransferQueue().waitIdle();

        m_device.freeCommandBuffers(transfer_cmd_pool, cmd_buffer);

        BRR_LogDebug("Immeadite copy buffers. Src buffer: {:#x}. Dst Buffer: {:#x}. Copy size: {}", size_t(VkBuffer(src_buffer->buffer)), size_t(VkBuffer(dst_buffer->buffer)), size);

        return true;
    }

    vk::DescriptorBufferInfo VulkanRenderDevice::GetBufferDescriptorInfo(BufferHandle buffer_handle, vk::DeviceSize size, vk::DeviceSize offset)
    {
        Buffer* buffer = m_buffer_alloc.GetResource(buffer_handle);

        return vk::DescriptorBufferInfo{buffer->buffer, offset, size};
    }

    VertexBufferHandle VulkanRenderDevice::CreateVertexBuffer(size_t buffer_size, VertexFormatFlags format, void* data)
    {
        VertexBufferHandle vertex_buffer_handle;
        VertexBuffer* vertex_buffer;
        // Create Vertex Buffer
        {
            const ResourceHandle resource_handle = m_vertex_buffer_alloc.CreateResource();
            if (!resource_handle)
            {
                return {};
            }
            vertex_buffer_handle.index = resource_handle.index; vertex_buffer_handle.validation = resource_handle.validation;
            vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
            assert(vertex_buffer && "VertexBuffer not initialized. Something is very wrong.");

            vk::SharingMode sharing_mode = IsDifferentTransferQueue() ? vk::SharingMode::eExclusive : vk::SharingMode::eExclusive;

            const vk::BufferUsageFlags vk_buffer_usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info
                .setUsage(vk_buffer_usage)
                .setSharingMode(sharing_mode)
                .setSize(buffer_size);
            if (sharing_mode == vk::SharingMode::eConcurrent)
            {
                std::array<uint32_t, 2> indices{
                    GetQueueFamilyIndices().m_graphicsFamily.value(),
                    GetQueueFamilyIndices().m_transferFamily.value()
                };
                buffer_create_info.setQueueFamilyIndices(indices);
            }

            VmaMemoryUsage vma_memory_usage = VMA_MEMORY_USAGE_AUTO;

            VmaAllocationCreateInfo alloc_create_info = {};
            alloc_create_info.usage = vma_memory_usage;
            alloc_create_info.flags = 0;
            alloc_create_info.requiredFlags = 0;
            alloc_create_info.preferredFlags = 0;
            alloc_create_info.memoryTypeBits = 0;
            alloc_create_info.pool = VK_NULL_HANDLE;
            alloc_create_info.pUserData = nullptr;
            alloc_create_info.priority = 1.0;

            VkBuffer new_buffer;
            VmaAllocation allocation;
            VmaAllocationInfo allocation_info;
            const vk::Result createBufferResult = vk::Result(vmaCreateBuffer(m_vma_allocator, reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &alloc_create_info,
                                                                             &new_buffer, &allocation, &allocation_info));

            if (createBufferResult != vk::Result::eSuccess)
            {
                m_vertex_buffer_alloc.DestroyResource(vertex_buffer_handle);
                BRR_LogError("Could not create VertexBuffer! Result code: {}.", vk::to_string(createBufferResult).c_str());
                return {};
            }
            vertex_buffer->buffer = new_buffer;
            vertex_buffer->buffer_allocation = allocation;
            vertex_buffer->allocation_info = allocation_info;
            vertex_buffer->buffer_size = buffer_size;
            vertex_buffer->buffer_format = format;

            BRR_LogDebug("Created vertex buffer. Buffer: {:#x}", (size_t)new_buffer);
        }

        if (data != nullptr)
        {
            UpdateBufferData(vertex_buffer->buffer, data, buffer_size, 0, 0);
        }

        return vertex_buffer_handle;
    }

    bool VulkanRenderDevice::DestroyVertexBuffer(VertexBufferHandle vertex_buffer_handle)
    {
        VertexBuffer* vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
        if (!vertex_buffer)
        {
            return false;
        }

        Frame& current_frame = m_frames[m_current_buffer];
        current_frame.buffer_delete_list.emplace_back(vertex_buffer->buffer, vertex_buffer->buffer_allocation);

        BRR_LogDebug("Destroyed vertex buffer. Buffer: {:#x}", (size_t)(static_cast<VkBuffer>(vertex_buffer->buffer)));

        return m_vertex_buffer_alloc.DestroyResource(vertex_buffer_handle);
    }

    bool VulkanRenderDevice::UpdateVertexBufferData(VertexBufferHandle vertex_buffer_handle, void* data, size_t data_size, uint32_t dst_offset)
    {
        const VertexBuffer* vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
        if (!vertex_buffer)
        {
            return false;
        }

        UpdateBufferData(vertex_buffer->buffer, data, data_size, 0, dst_offset);

        return true;
    }

    bool VulkanRenderDevice::BindVertexBuffer(VertexBufferHandle vertex_buffer_handle)
    {
        const VertexBuffer* vertex_buffer = m_vertex_buffer_alloc.GetResource(vertex_buffer_handle);
        if (!vertex_buffer)
        {
            return false;
        }
        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();

        uint32_t offset = 0;

        command_buffer.bindVertexBuffers(0, vertex_buffer->buffer, offset);
        return true;
    }

    IndexBufferHandle VulkanRenderDevice::CreateIndexBuffer(size_t buffer_size, IndexType format, void* data)
    {
        IndexBufferHandle index_buffer_handle;
        IndexBuffer* index_buffer;
        // Create Vertex Buffer
        {
            const ResourceHandle resource_handle = m_index_buffer_alloc.CreateResource();
            if (!resource_handle)
            {
                return {};
            }
            index_buffer_handle.index = resource_handle.index; index_buffer_handle.validation = resource_handle.validation;
            index_buffer = m_index_buffer_alloc.GetResource(index_buffer_handle);
            assert(index_buffer && "IndexBuffer not initialized. Something is very wrong.");

            vk::SharingMode sharing_mode = IsDifferentTransferQueue() ? vk::SharingMode::eExclusive : vk::SharingMode::eExclusive;

            const vk::BufferUsageFlags vk_buffer_usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;

            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info
                .setUsage(vk_buffer_usage)
                .setSharingMode(sharing_mode)
                .setSize(buffer_size);
            if (sharing_mode == vk::SharingMode::eConcurrent)
            {
                std::array<uint32_t, 2> indices{
                    GetQueueFamilyIndices().m_graphicsFamily.value(),
                    GetQueueFamilyIndices().m_transferFamily.value()
                };
                buffer_create_info.setQueueFamilyIndices(indices);
            }

            VmaMemoryUsage vma_memory_usage = VMA_MEMORY_USAGE_AUTO;

            VmaAllocationCreateInfo alloc_create_info = {};
            alloc_create_info.usage = vma_memory_usage;
            alloc_create_info.flags = 0;
            alloc_create_info.requiredFlags = 0;
            alloc_create_info.preferredFlags = 0;
            alloc_create_info.memoryTypeBits = 0;
            alloc_create_info.pool = VK_NULL_HANDLE;
            alloc_create_info.pUserData = nullptr;
            alloc_create_info.priority = 1.0;

            VkBuffer new_buffer;
            VmaAllocation allocation;
            VmaAllocationInfo allocation_info;
            const vk::Result createBufferResult = vk::Result(vmaCreateBuffer(m_vma_allocator, reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &alloc_create_info,
                                                                             &new_buffer, &allocation, &allocation_info));

            if (createBufferResult != vk::Result::eSuccess)
            {
                m_index_buffer_alloc.DestroyResource(index_buffer_handle);
                BRR_LogError("Could not create IndexBuffer! Result code: {}.", vk::to_string(createBufferResult).c_str());
                return {};
            }
            index_buffer->buffer = new_buffer;
            index_buffer->buffer_allocation = allocation;
            index_buffer->allocation_info = allocation_info;
            index_buffer->buffer_size = buffer_size;
            index_buffer->buffer_format = format;

            BRR_LogDebug("Created index buffer. Buffer: {:#x}", (size_t)new_buffer);
        }

        if (data != nullptr)
        {
            UpdateBufferData(index_buffer->buffer, data, buffer_size, 0, 0);
        }

        return index_buffer_handle;
    }

    bool VulkanRenderDevice::DestroyIndexBuffer(IndexBufferHandle index_buffer_handle)
    {
        IndexBuffer* index_buffer = m_index_buffer_alloc.GetResource(index_buffer_handle);
        if (!index_buffer)
        {
            return false;
        }

        Frame& current_frame = m_frames[m_current_buffer];
        current_frame.buffer_delete_list.emplace_back(index_buffer->buffer, index_buffer->buffer_allocation);

        BRR_LogDebug("Destroyed index buffer. Buffer: {:#x}", (size_t)(static_cast<VkBuffer>(index_buffer->buffer)));

        return m_index_buffer_alloc.DestroyResource(index_buffer_handle);
    }

    bool VulkanRenderDevice::UpdateIndexBufferData(IndexBufferHandle index_buffer_handle, void* data, size_t data_size,
                                                   uint32_t dst_offset)
    {
        const IndexBuffer* index_buffer = m_index_buffer_alloc.GetResource(index_buffer_handle);
        if (!index_buffer)
        {
            return false;
        }

        UpdateBufferData(index_buffer->buffer, data, data_size, 0, dst_offset);

        return true;
    }

    bool VulkanRenderDevice::BindIndexBuffer(IndexBufferHandle index_buffer_handle)
    {
        const IndexBuffer* index_buffer = m_index_buffer_alloc.GetResource(index_buffer_handle);
        if (!index_buffer)
        {
            return false;
        }

        vk::CommandBuffer command_buffer = GetCurrentGraphicsCommandBuffer();

        vk::IndexType index_type;
        switch (index_buffer->buffer_format)
        {
        case IndexType::UINT8: 
            index_type = vk::IndexType::eUint8EXT;
            break;
        case IndexType::UINT16: 
            index_type = vk::IndexType::eUint16;
            break;
        case IndexType::UINT32: 
            index_type = vk::IndexType::eUint32;
            break;
        default: 
            index_type = vk::IndexType::eNoneKHR;
            break;
        }

        command_buffer.bindIndexBuffer(index_buffer->buffer, 0, index_type);

        return true;
    }

    Texture2DHandle VulkanRenderDevice::Create_Texture2D(size_t width, size_t height, ImageUsage image_usage, DataFormat image_format)
    {
        Texture2DHandle texture2d_handle;
        Texture2D* texture2d;
        const ResourceHandle resource_handle = m_texture2d_alloc.CreateResource();
        if (!resource_handle)
        {
            return {};
        }
        texture2d_handle.index = resource_handle.index; texture2d_handle.validation = resource_handle.validation;
        texture2d = m_texture2d_alloc.GetResource(texture2d_handle);
        assert(texture2d && "Texture2D not initialized. Something is very wrong.");
        
        vk::ImageUsageFlags vk_image_usage = VkImageUsageFromDeviceImageUsage(image_usage);
        vk::Format vk_format = VkFormatFromDeviceDataFormat(image_format);

        vk::ImageCreateInfo img_create_info;
        img_create_info
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setUsage(vk_image_usage)
            .setExtent(vk::Extent3D(width, height, 1))
            .setFormat(vk_format)
            .setImageType(vk::ImageType::e2D)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setMipLevels(1)
            .setArrayLayers(1);

        VmaAllocationCreateInfo alloc_create_info;
        alloc_create_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_create_info.flags = 0;
        alloc_create_info.requiredFlags = 0;
        alloc_create_info.preferredFlags = 0;
        alloc_create_info.memoryTypeBits = 0;
        alloc_create_info.pool = VK_NULL_HANDLE;
        alloc_create_info.pUserData = nullptr;
        alloc_create_info.priority = 1.0;

        VkImage new_image;
        VmaAllocation allocation;
        VmaAllocationInfo allocation_info;
        const vk::Result createImageResult = vk::Result(vmaCreateImage(m_vma_allocator,
                                                                       reinterpret_cast<VkImageCreateInfo*>
                                                                       (&img_create_info),
                                                                       &alloc_create_info, &new_image,
                                                                       &allocation, &allocation_info));
        if (createImageResult != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create Image! Result code: {}.", vk::to_string(createImageResult).c_str());
            return {};
        }

        BRR_LogInfo("Image created.");

        // Create ImageView

        vk::ImageSubresourceRange subresource_range;
        subresource_range
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1);

        vk::ImageViewCreateInfo view_create_info;
        view_create_info
            .setImage(new_image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk_format)
            .setSubresourceRange(subresource_range);

        auto view_result = m_device.createImageView(view_create_info);
        if (view_result.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create ImageView! Result code: {}.", vk::to_string(view_result.result).c_str());
            vmaDestroyImage(m_vma_allocator, new_image, allocation);
            return {};
        }

        texture2d->image = new_image;
        texture2d->image_view = view_result.value;
        texture2d->image_allocation = allocation;
        texture2d->allocation_info = allocation_info;
        texture2d->width = width;
        texture2d->height = height;
        texture2d->image_format = vk_format;

        if ((image_usage & ImageUsage::SampledImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        }
        //else if ((image_usage & ImageUsage::StorageImage) != 0)
        //{
        //    // TODO
        //    //texture2d->image_layout = vk::ImageLayout::
        //}
        else if ((image_usage & ImageUsage::ColorAttachmentImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eColorAttachmentOptimal;
        }
        else if ((image_usage & ImageUsage::DepthStencilAttachmentImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }
        else if ((image_usage & ImageUsage::TransferSrcImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eTransferSrcOptimal;
        }
        else if ((image_usage & ImageUsage::TransferDstImage) != 0)
        {
            texture2d->image_layout = vk::ImageLayout::eTransferDstOptimal;
        }

        return texture2d_handle;
    }

    bool VulkanRenderDevice::DestroyTexture2D(Texture2DHandle texture2d_handle)
    {
        const Texture2D* texture = m_texture2d_alloc.GetResource(texture2d_handle);
        if (!texture)
        {
            return false;
        }

        Frame& current_frame = m_frames[m_current_buffer];
        current_frame.texture_delete_list.emplace_back(texture->image, texture->image_view, texture->image_allocation);

        BRR_LogDebug("Destroyed Texture2D. Image: {:#x}", (size_t)(static_cast<VkImage>(texture->image)));

        return m_texture2d_alloc.DestroyResource(texture2d_handle);
    }

    bool VulkanRenderDevice::UpdateTexture2DData(Texture2DHandle texture2d_handle, const void* data, size_t buffer_size,
                                                 const glm::ivec2& image_offset, const glm::uvec2& image_extent)
    {
        const Texture2D* texture = m_texture2d_alloc.GetResource(texture2d_handle);
        if (!texture)
        {
            return false;
        }

        vk::CommandBuffer cmd_buffer = GetCurrentGraphicsCommandBuffer();
        vk::CommandBuffer transfer_cmd_buffer = GetCurrentTransferCommandBuffer();

        {
            TransitionImageLayout(transfer_cmd_buffer, texture->image,
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                  vk::AccessFlagBits2::eNone, vk::PipelineStageFlagBits2::eNone,
                                  vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer,
                                  vk::ImageAspectFlagBits::eColor);
        }

        for (uint32_t y = 0; y < image_extent.y; y += IMAGE_TRANSFER_BLOCK_SIZE)
        {
            for (uint32_t x = 0; x < image_extent.x; x += IMAGE_TRANSFER_BLOCK_SIZE)
            {
                glm::uvec2 block_size = {
                    std::min(IMAGE_TRANSFER_BLOCK_SIZE, image_extent.x - x),
                    std::min(IMAGE_TRANSFER_BLOCK_SIZE, image_extent.y - y)
                };
                uint32_t block_size_bytes = block_size.x * block_size.y * 4;
                StagingBufferHandle staging_buffer{};
                m_staging_allocator.AllocateStagingBuffer(m_current_frame, block_size_bytes, &staging_buffer, false);

                m_staging_allocator.WriteBlockImageToStaging (staging_buffer, static_cast<const uint8_t*>(data), {x, y}, block_size, image_extent, 4); //(staging_buffer, 0, buffer.data(), block_size_bytes);

                m_staging_allocator.CopyFromStagingToImage(staging_buffer, texture->image, {block_size.x, block_size.y, 1},
                                                           0, {static_cast<int32_t>(x), static_cast<int32_t>(y), 0});
            }
        }

        if (IsDifferentTransferQueue())
        {
            TransitionImageLayout(transfer_cmd_buffer, texture->image,
                                  vk::ImageLayout::eTransferDstOptimal, texture->image_layout,
                                  vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eNone, vk::PipelineStageFlagBits2::eNone,
                                  vk::ImageAspectFlagBits::eColor, GetQueueFamilyIndices().m_transferFamily.value(),
                                  GetQueueFamilyIndices().m_graphicsFamily.value());

            TransitionImageLayout(cmd_buffer, texture->image,
                                  vk::ImageLayout::eTransferDstOptimal, texture->image_layout,
                                  vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::ImageAspectFlagBits::eColor, GetQueueFamilyIndices().m_transferFamily.value(),
                                  GetQueueFamilyIndices().m_graphicsFamily.value());
        }
        else
        {
            TransitionImageLayout(cmd_buffer, texture->image,
                                  vk::ImageLayout::eTransferDstOptimal, texture->image_layout,
                                  vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::ImageAspectFlagBits::eColor);
        }
        return true;
    }

    bool VulkanRenderDevice::TransitionImageLayout(vk::CommandBuffer cmd_buffer, vk::Image image,
                                                   vk::ImageLayout current_layout, vk::ImageLayout new_layout,
                                                   vk::AccessFlags2 src_access_mask,
                                                   vk::PipelineStageFlags2 src_stage_mask,
                                                   vk::AccessFlags2 dst_access_mask,
                                                   vk::PipelineStageFlags2 dst_stage_mask,
                                                   vk::ImageAspectFlags image_aspect,
                                                   uint32_t src_queue_index, uint32_t dst_queue_index)
    {
        vk::ImageSubresourceRange img_subresource_range;
        img_subresource_range
            .setAspectMask(image_aspect)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1);

        vk::ImageMemoryBarrier2 img_mem_barrier;
        img_mem_barrier
            .setOldLayout(current_layout)
            .setNewLayout(new_layout)
            .setSrcAccessMask(src_access_mask)
            .setDstAccessMask(dst_access_mask)
            .setSrcStageMask(src_stage_mask)
            .setDstStageMask(dst_stage_mask)
            .setSrcQueueFamilyIndex(src_queue_index)
            .setDstQueueFamilyIndex(dst_queue_index)
            .setImage(image)
            .setSubresourceRange(img_subresource_range);

        vk::DependencyInfo dependency_info;
        dependency_info
            .setImageMemoryBarriers(img_mem_barrier);

        cmd_buffer.pipelineBarrier2(dependency_info);

        return true;
    }

    vk::DescriptorImageInfo VulkanRenderDevice::GetImageDescriptorInfo(Texture2DHandle texture2d_handle)
    {
        Texture2D* texture = m_texture2d_alloc.GetResource(texture2d_handle);

        return vk::DescriptorImageInfo{m_texture2DSampler, texture->image_view, vk::ImageLayout::eShaderReadOnlyOptimal};
    }

    ResourceHandle VulkanRenderDevice::Create_GraphicsPipeline(const Shader& shader, 
                                                               const std::vector<DataFormat>& color_attachment_formats,
                                                               DataFormat depth_attachment_format)
    {
        GraphicsPipeline* graphics_pipeline;
        const ResourceHandle pipeline_handle = m_graphics_pipeline_alloc.CreateResource();
        if (!pipeline_handle)
        {
            return {};
        }
        graphics_pipeline = m_graphics_pipeline_alloc.GetResource(pipeline_handle);

        vk::PipelineVertexInputStateCreateInfo vertex_input_info = shader.GetPipelineVertexInputState();

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false);

		vk::PipelineViewportStateCreateInfo viewport_state_info{};
		viewport_state_info
			.setViewportCount(1)
		    .setScissorCount(1);

		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info {};
		depth_stencil_state_create_info
		    .setDepthTestEnable(VK_TRUE)
            .setDepthWriteEnable(VK_TRUE)
            .setDepthCompareOp(vk::CompareOp::eLess)
		    .setDepthBoundsTestEnable(VK_FALSE)
		    .setMinDepthBounds(0.0)
		    .setMaxDepthBounds(1.0)
            .setStencilTestEnable(VK_FALSE);

		vk::PipelineRasterizationStateCreateInfo rasterization_state_info{};
		rasterization_state_info
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1.f)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setDepthBiasEnable(false)
			.setDepthBiasConstantFactor(0.f)
			.setDepthBiasClamp(0.f)
			.setDepthBiasSlopeFactor(0.f);

		vk::PipelineMultisampleStateCreateInfo multisampling_info{};
		multisampling_info
			.setSampleShadingEnable(false)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setMinSampleShading(1.f)
			.setPSampleMask(nullptr)
			.setAlphaToCoverageEnable(false)
			.setAlphaToOneEnable(false);

		vk::PipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
			.setBlendEnable(false)
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
			.setAlphaBlendOp(vk::BlendOp::eAdd);

		vk::PipelineColorBlendStateCreateInfo color_blending_info{};
		color_blending_info
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachments(color_blend_attachment);

#if 1
		std::vector<vk::DynamicState> dynamic_states{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
		dynamic_state_info
			.setDynamicStates(dynamic_states);
#endif

		vk::PipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info
			.setSetLayouts(shader.GetDescriptorSetLayouts());

		auto createPipelineLayoutResult = m_device.createPipelineLayout(pipeline_layout_info);
		if (createPipelineLayoutResult.result != vk::Result::eSuccess)
		{
			BRR_LogError("Not able to create PipelineLayout. Result code: {}.", vk::to_string(createPipelineLayoutResult.result).c_str());
            m_graphics_pipeline_alloc.DestroyResource(pipeline_handle);
			return {};
		}
		graphics_pipeline->pipeline_layout = createPipelineLayoutResult.value;

		std::vector<vk::Format> vk_color_attachment_formats;
		vk_color_attachment_formats.reserve(color_attachment_formats.size());
		for (const auto& format : color_attachment_formats)
		{
		    vk::Format vk_format = VKRD::VkFormatFromDeviceDataFormat(format);
			vk_color_attachment_formats.emplace_back(vk_format);
		}
		vk::Format vk_depth_attachment_format = VKRD::VkFormatFromDeviceDataFormat(depth_attachment_format);

		vk::PipelineRenderingCreateInfo rendering_create_info {};
		rendering_create_info
		    .setColorAttachmentFormats(vk_color_attachment_formats)
            .setDepthAttachmentFormat(vk_depth_attachment_format);

		vk::GraphicsPipelineCreateInfo graphics_pipeline_info{};
		graphics_pipeline_info
			.setStages(shader.GetPipelineStagesInfo())
			.setPVertexInputState(&vertex_input_info)
			.setPInputAssemblyState(&input_assembly_info)
		    .setPDynamicState(&dynamic_state_info)
			.setPViewportState(&viewport_state_info)
			.setPRasterizationState(&rasterization_state_info)
			.setPMultisampleState(&multisampling_info)
			.setPColorBlendState(&color_blending_info)
            .setPDepthStencilState(&depth_stencil_state_create_info);
		graphics_pipeline_info
		    .setPNext(&rendering_create_info)
			.setLayout(graphics_pipeline->pipeline_layout)
			.setSubpass(0)
			.setBasePipelineHandle(VK_NULL_HANDLE)
			.setBasePipelineIndex(-1);

		auto createGraphicsPipelineResult = m_device.createGraphicsPipeline(VK_NULL_HANDLE, graphics_pipeline_info);
		if (createGraphicsPipelineResult.result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not create GraphicsPipeline! Result code: {}.", vk::to_string(createGraphicsPipelineResult.result).c_str());
            m_graphics_pipeline_alloc.DestroyResource(pipeline_handle);
			return {};
		}

		graphics_pipeline->pipeline = createGraphicsPipelineResult.value;

		BRR_LogInfo("Graphics DevicePipeline created.");

        return pipeline_handle;
    }

    bool VulkanRenderDevice::DestroyGraphicsPipeline(ResourceHandle graphics_pipeline_handle)
    {
        const GraphicsPipeline* graphics_pipeline = m_graphics_pipeline_alloc.GetResource(graphics_pipeline_handle);
        if (!graphics_pipeline)
        {
            return false;
        }

        m_device.destroyPipeline(graphics_pipeline->pipeline);
		m_device.destroyPipelineLayout(graphics_pipeline->pipeline_layout);

        m_graphics_pipeline_alloc.DestroyResource(graphics_pipeline_handle);

        return true;
    }

    void VulkanRenderDevice::Bind_GraphicsPipeline(ResourceHandle graphics_pipeline_handle)
    {
        const GraphicsPipeline* graphics_pipeline = m_graphics_pipeline_alloc.GetResource(graphics_pipeline_handle);
        if (!graphics_pipeline)
        {
            return;
        }

        Frame& current_frame = m_frames[m_current_buffer];

        current_frame.graphics_cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline->pipeline);
    }

    void VulkanRenderDevice::Bind_DescriptorSet(ResourceHandle graphics_pipeline_handle, vk::DescriptorSet descriptor_set, uint32_t set_index)
    {
        const GraphicsPipeline* graphics_pipeline = m_graphics_pipeline_alloc.GetResource(graphics_pipeline_handle);
        if (!graphics_pipeline)
        {
            return;
        }

        Frame& current_frame = m_frames[m_current_buffer];

        current_frame.graphics_cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                             graphics_pipeline->pipeline_layout, set_index,
                                                             descriptor_set, {});
    }

    void VulkanRenderDevice::Draw(uint32_t num_vertex, uint32_t num_instances, uint32_t first_vertex,
                                  uint32_t first_instance)
    {
        Frame& current_frame = m_frames[m_current_buffer];

        current_frame.graphics_cmd_buffer.draw(num_vertex, num_instances, first_vertex, first_instance);
    }

    void VulkanRenderDevice::DrawIndexed(uint32_t num_indices, uint32_t num_instances, uint32_t first_index,
                                         uint32_t vertex_offset, uint32_t first_instance)
    {
        Frame& current_frame = m_frames[m_current_buffer];

        current_frame.graphics_cmd_buffer.drawIndexed(num_indices, num_instances, first_index, vertex_offset, first_instance);
    }

    void VulkanRenderDevice::UpdateBufferData(vk::Buffer dst_buffer, void* data, size_t size,
                                              uint32_t src_offset, uint32_t dst_offset)
    {
        { // Make transfer
            uint32_t written_bytes = 0;
            while (written_bytes != size)
            {
                render::StagingBufferHandle staging_buffer{};
                const uint32_t allocated = m_staging_allocator.AllocateStagingBuffer(m_current_frame, size - written_bytes, &staging_buffer);

                m_staging_allocator.WriteLinearBufferToStaging(staging_buffer, 0, static_cast<char*>(data) + written_bytes, allocated);

                m_staging_allocator.CopyFromStagingToBuffer(staging_buffer, dst_buffer, allocated, src_offset, dst_offset + written_bytes);

                written_bytes += allocated;
            }
        }

        vk::CommandBuffer transfer_cmd_buffer = GetCurrentTransferCommandBuffer();
        vk::CommandBuffer grapics_cmd_buffer = GetCurrentGraphicsCommandBuffer();

        if (IsDifferentTransferQueue())
        {
            vk::BufferMemoryBarrier2 buffer_memory_barrier;
            buffer_memory_barrier
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setSrcQueueFamilyIndex(GetQueueFamilyIndices().m_transferFamily.value())
                .setDstQueueFamilyIndex(GetQueueFamilyIndices().m_graphicsFamily.value())
                .setBuffer(dst_buffer)
                .setSize(size);

            vk::DependencyInfo dependency_info;
            dependency_info
                .setBufferMemoryBarriers(buffer_memory_barrier);

            transfer_cmd_buffer.pipelineBarrier2(dependency_info);

            buffer_memory_barrier
                .setDstStageMask(vk::PipelineStageFlagBits2::eVertexInput)
                .setDstAccessMask(vk::AccessFlagBits2::eVertexAttributeRead)
                .setSrcQueueFamilyIndex(GetQueueFamilyIndices().m_transferFamily.value())
                .setDstQueueFamilyIndex(GetQueueFamilyIndices().m_graphicsFamily.value())
                .setBuffer(dst_buffer)
                .setSize(size);

            dependency_info
                .setBufferMemoryBarriers(buffer_memory_barrier);

            grapics_cmd_buffer.pipelineBarrier2(dependency_info);
        }
        else
        {
            // TODO
            vk::MemoryBarrier memory_barrier;
            memory_barrier
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead);

            transfer_cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                                vk::PipelineStageFlagBits::eVertexInput,
                                                vk::DependencyFlags(), 1, &memory_barrier, 0,
                                                nullptr, 0, nullptr);
        }
    }

    void VulkanRenderDevice::Init_VkInstance(vis::Window* window)
    {
        // Dynamic load of the library
        {
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = VulkanDynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            if (vkGetInstanceProcAddr == nullptr)
            {
                BRR_LogError("Could not load 'vkGetInstanceProcAddr' function address. Can't create vkInstance.");
                exit(1);
            }
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
            BRR_LogDebug("Loaded 'vkGetInstanceProcAddr' function address. Proceeding to create vkInstance.");
        }

        std::vector<char const*> instance_layers;
        // Check for validation layers support
#ifdef NDEBUG
        constexpr bool use_validation_layers = false;
#else
        constexpr bool use_validation_layers = true;
        instance_layers.emplace_back("VK_LAYER_KHRONOS_validation");
        BRR_LogDebug("Using validation layers.");
#endif

        std::vector<char const*> enabled_layers;
        if (use_validation_layers)
        {
            auto enumInstLayerPropsResult = vk::enumerateInstanceLayerProperties();
            std::vector<vk::LayerProperties> layers = enumInstLayerPropsResult.value;
            std::vector<char const*> acceptedLayers;
            if (VkHelpers::CheckLayers(instance_layers, layers, acceptedLayers))
            {
                enabled_layers = acceptedLayers;
            }
            else
            {
                BRR_LogError("Could not find any of the required validation layers.");
                /*exit(1);*/
            }

            {
                LogStreamBuffer log_msg = BRR_DebugStrBuff();
                log_msg << "Available Layers:";
                for (vk::LayerProperties& layer : layers)
                {
                    log_msg << "\n\tLayer name: " << layer.layerName;
                }
            }
        }

        // Gather required extensions
        std::vector<const char*> extensions{};
        {
            window->GetRequiredVulkanExtensions(extensions);
#ifndef NDEBUG
            extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

            // TODO: Check if the required extensions are supported by Vulkan
            uint32_t extension_count = 0;
            auto enumInstExtPropsResult = vk::enumerateInstanceExtensionProperties();
            std::vector<vk::ExtensionProperties> extension_properties = enumInstExtPropsResult.value;

            {
              LogStreamBuffer log_msg = BRR_DebugStrBuff();
                  log_msg << "Available Instance Extensions:";
                for (vk::ExtensionProperties& extension : extension_properties)
                {
                    log_msg << "\n\tExtension name: " << extension.extensionName;
                }
            }

          {
            LogStreamBuffer aLogMsg = BRR_InfoStrBuff();
                aLogMsg << "Required Instance Extensions:";
                for (const char* extension : extensions)
                {
                  aLogMsg << "\n\tExtension name: " << extension;
                }
                aLogMsg.Flush();
          }
        }

        vk::ApplicationInfo app_info{ };
        app_info.setPApplicationName("Brayner Renderer");
        app_info.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
        app_info.setApiVersion(VK_API_VERSION_1_3);

        vk::InstanceCreateInfo inst_create_info{};
        inst_create_info
            .setPApplicationInfo(&app_info)
            .setPEnabledExtensionNames(extensions)
            .setPEnabledLayerNames(enabled_layers);

         auto createInstanceResult = vk::createInstance(inst_create_info);
         if (createInstanceResult.result != vk::Result::eSuccess)
         {
             BRR_LogError("Could not create Vulkan Instance! Result code: {}.", vk::to_string(createInstanceResult.result).c_str());
             exit(1);
         }
         m_vulkan_instance = createInstanceResult.value;

        {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vulkan_instance);
            BRR_LogInfo("Loaded instance specific vulkan functions addresses.");
        }

         BRR_LogDebug("Instance Created");
    }

    void VulkanRenderDevice::Init_PhysDevice(vk::SurfaceKHR surface)
    {
        auto enumPhysDevicesResult = m_vulkan_instance.enumeratePhysicalDevices();
        std::vector<vk::PhysicalDevice> devices = enumPhysDevicesResult.value;

        if (devices.size() == 0)
        {
            BRR_LogError("Failed to find a physical device with Vulkan support. Exitting program.");
            exit(1);
        }

        {
            LogStreamBuffer log_msg = BRR_DebugStrBuff();
            log_msg << "Available devices:";
            for (vk::PhysicalDevice& device : devices)
            {
                log_msg << "\n\tDevice: " << device.getProperties().deviceName;
            }
        }

        m_phys_device = VkHelpers::Select_PhysDevice(devices, surface);
        std::string device_name = m_phys_device.getProperties().deviceName;

        auto device_extensions = m_phys_device.enumerateDeviceExtensionProperties();
        if (device_extensions.result == vk::Result::eSuccess)
        {
            LogStreamBuffer log_msg = BRR_DebugStrBuff();
            log_msg << "Available Device Extensions (" << device_name << "):";
            for (vk::ExtensionProperties& extension : device_extensions.value)
            {
                log_msg << "\n\tExtension name: " << extension.extensionName;
            }
        }

        m_device_properties = m_phys_device.getProperties2();

        BRR_LogInfo("Selected physical device: {}", m_phys_device.getProperties().deviceName);
    }

    void VulkanRenderDevice::Init_Queues_Indices(vk::SurfaceKHR surface)
    {
        // Check for queue families
        m_queue_family_indices = VkHelpers::Find_QueueFamilies(m_phys_device, surface);

        if (!m_queue_family_indices.m_graphicsFamily.has_value())
        {
            BRR_LogError("Failed to find graphics family queue. Exitting program.");
            exit(1);
        }

        if (!m_queue_family_indices.m_presentFamily.has_value())
        {
            BRR_LogError("Failed to find presentation family queue. Exitting program.");
            exit(1);
        }
    }

    void VulkanRenderDevice::Init_Device()
    {
        if (!m_queue_family_indices.m_graphicsFamily.has_value())
        {
            BRR_LogError("Cannot create device without initializing at least the graphics queue.");
            exit(1);
        }

        const uint32_t graphics_family_idx = m_queue_family_indices.m_graphicsFamily.value();
        const uint32_t presentation_family_idx = m_queue_family_indices.m_presentFamily.value();
        const uint32_t transfer_family_idx = m_queue_family_indices.m_transferFamily.has_value() ? m_queue_family_indices.m_transferFamily.value() : graphics_family_idx;

        BRR_LogDebug("Selected Queue Families:\n"
                     "\tGraphics Queue Family:\t {}\n"
                     "\tPresent Queue Family:\t {}\n"
                     "\tTransfer Queue Family:\t {}",
                     graphics_family_idx,
                     presentation_family_idx,
                     transfer_family_idx);

        float priorities = 1.0;
        std::vector<vk::DeviceQueueCreateInfo> queues;

        queues.push_back(vk::DeviceQueueCreateInfo{}
            .setQueueFamilyIndex(graphics_family_idx)
            .setQueuePriorities(priorities));

        m_different_present_queue = graphics_family_idx != presentation_family_idx;
        if (m_different_present_queue)
        {
            queues.push_back(vk::DeviceQueueCreateInfo{}
                .setQueueFamilyIndex(presentation_family_idx)
                .setQueuePriorities(priorities));
        }

        m_different_transfer_queue = graphics_family_idx != transfer_family_idx;
        if (m_different_transfer_queue)
        {
            queues.push_back(vk::DeviceQueueCreateInfo{}
                .setQueueFamilyIndex(transfer_family_idx)
                .setQueuePriorities(priorities));
        }

        vk::PhysicalDeviceFeatures device_features{};
        device_features.setSamplerAnisotropy(VK_TRUE);

        std::vector<const char*> device_extensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        vk::PhysicalDeviceSynchronization2Features synchronization2_features {true};

        vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features {true, &synchronization2_features};

        vk::DeviceCreateInfo device_create_info = vk::DeviceCreateInfo{};
        device_create_info
            .setPNext(&dynamic_rendering_features)
            .setQueueCreateInfos(queues)
            .setPEnabledFeatures(&device_features)
            .setEnabledLayerCount(0)
            .setPEnabledExtensionNames(device_extensions);

        auto createDeviceResult = m_phys_device.createDevice(device_create_info);
        if (createDeviceResult.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create Vulkan Device! Result code: {}.", vk::to_string(createDeviceResult.result).c_str());
            exit(1);
        }
        m_device = createDeviceResult.value;
        {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
            BRR_LogDebug("Loaded Device specific Vulkan functions addresses.");
        }

        m_graphics_queue = m_device.getQueue(graphics_family_idx, 0);

        m_presentation_queue = (m_different_present_queue) ? m_device.getQueue(presentation_family_idx, 0) : m_graphics_queue;

        m_transfer_queue = (m_different_transfer_queue) ? m_device.getQueue(transfer_family_idx, 0) : m_graphics_queue;

        BRR_LogDebug("VkDevice Created");
    }

    void VulkanRenderDevice::Init_Allocator()
    {
        VmaVulkanFunctions vulkan_functions;
        // Initialize function pointers
        {
            vulkan_functions.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
            vulkan_functions.vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;
            vulkan_functions.vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties;
            vulkan_functions.vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties;
            vulkan_functions.vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory;
            vulkan_functions.vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory;
            vulkan_functions.vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory;
            vulkan_functions.vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory;
            vulkan_functions.vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges;
            vulkan_functions.vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges;
            vulkan_functions.vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory;
            vulkan_functions.vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory;
            vulkan_functions.vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements;
            vulkan_functions.vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements;
            vulkan_functions.vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer;
            vulkan_functions.vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer;
            vulkan_functions.vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage;
            vulkan_functions.vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage;
            vulkan_functions.vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer;
            vulkan_functions.vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2;
            vulkan_functions.vkGetImageMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2;
            vulkan_functions.vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2;
            vulkan_functions.vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2;
            vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2;
            vulkan_functions.vkGetDeviceBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceBufferMemoryRequirements;
            vulkan_functions.vkGetDeviceImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceImageMemoryRequirements;
        }

        VmaAllocatorCreateInfo vma_alloc_create_info {};
        vma_alloc_create_info.device = m_device;
        vma_alloc_create_info.instance = m_vulkan_instance;
        vma_alloc_create_info.physicalDevice = m_phys_device;
        vma_alloc_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
        vma_alloc_create_info.pVulkanFunctions = &vulkan_functions;

        vmaCreateAllocator(&vma_alloc_create_info, &m_vma_allocator);

        BRR_LogDebug("Initialized VMA allocator. Allocator: {:#x}.", (size_t)m_vma_allocator);
    }

    void VulkanRenderDevice::Init_CommandPool()
    {
        vk::CommandPoolCreateInfo command_pool_info{};
        command_pool_info
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(m_queue_family_indices.m_graphicsFamily.value());

         auto createCmdPoolResult = m_device.createCommandPool(command_pool_info);
         if (createCmdPoolResult.result != vk::Result::eSuccess)
         {
             BRR_LogError("Could not create CommandPool! Result code: {}.", vk::to_string(createCmdPoolResult.result).c_str());
             exit(1);
         }
         m_graphics_command_pool = createCmdPoolResult.value;

         BRR_LogInfo("CommandPool created.");

        if (m_different_present_queue)
        {
            vk::CommandPoolCreateInfo present_pool_info{};
            present_pool_info
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(m_queue_family_indices.m_presentFamily.value());

             auto createPresentCmdPoolResult = m_device.createCommandPool(present_pool_info);
             if (createPresentCmdPoolResult.result != vk::Result::eSuccess)
             {
                 BRR_LogError("Could not create present CommandPool! Result code: {}.", vk::to_string(createPresentCmdPoolResult.result).c_str());
                 exit(1);
             }
             m_present_command_pool = createPresentCmdPoolResult.value;

             BRR_LogInfo("Separate Present CommandPool created.");
        }

        if (m_different_transfer_queue)
        {
            vk::CommandPoolCreateInfo transfer_pool_info{};
            transfer_pool_info
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(m_queue_family_indices.m_transferFamily.value());

             auto createTransferCommandPoolResult = m_device.createCommandPool(transfer_pool_info);
             if (createTransferCommandPoolResult.result != vk::Result::eSuccess)
             {
                 BRR_LogError("Could not create transfer CommandPool! Result code: {}.", vk::to_string(createTransferCommandPoolResult.result).c_str());
                 exit(1);
             }
             m_transfer_command_pool = createTransferCommandPoolResult.value;

             BRR_LogInfo("Separate Transfer CommandPool created.");
        }
    }

    void VulkanRenderDevice::Init_Frames()
    {
        std::array<vk::CommandBuffer, FRAME_LAG> graphics_cmd_buffers;
        std::array<vk::CommandBuffer, FRAME_LAG> transfer_cmd_buffers;

        allocateCommandBuffer(m_device, m_graphics_command_pool, vk::CommandBufferLevel::ePrimary, FRAME_LAG, graphics_cmd_buffers.data());
        allocateCommandBuffer(m_device, m_transfer_command_pool, vk::CommandBufferLevel::ePrimary, FRAME_LAG, transfer_cmd_buffers.data());

        for (size_t idx = 0; idx < FRAME_LAG; ++idx)
        {
            m_frames[idx].graphics_cmd_buffer = graphics_cmd_buffers[idx];
            m_frames[idx].transfer_cmd_buffer = transfer_cmd_buffers[idx];

            // Render finished semaphores
            {
                auto createRenderFinishedSempahoreResult = m_device.createSemaphore(vk::SemaphoreCreateInfo{});
                if (createRenderFinishedSempahoreResult.result != vk::Result::eSuccess)
                {
                    BRR_LogError("Could not create Render Finished Semaphore for swapchain! Result code: {}.", vk::to_string(createRenderFinishedSempahoreResult.result).c_str());
                    exit(1);
                }

                m_frames[idx].render_finished_semaphore = createRenderFinishedSempahoreResult.value;
            }
            // Transfer finished semaphores
            {
                auto createTransferFinishedSempahoreResult = m_device.createSemaphore(vk::SemaphoreCreateInfo{});
                if (createTransferFinishedSempahoreResult.result != vk::Result::eSuccess)
                {
                    BRR_LogError("Could not create Transfer Finished Semaphore for swapchain! Result code: {}.", vk::to_string(createTransferFinishedSempahoreResult.result).c_str());
                    exit(1);
                }
                m_frames[idx].transfer_finished_semaphore = createTransferFinishedSempahoreResult.value;
            }
        }
    }

    void VulkanRenderDevice::Init_Texture2DSampler()
    {
        vk::SamplerCreateInfo sampler_create_info;
        sampler_create_info
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setAnisotropyEnable(VK_TRUE)
            .setMaxAnisotropy(m_device_properties.properties.limits.maxSamplerAnisotropy)
            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
            .setUnnormalizedCoordinates(VK_FALSE)
            .setCompareEnable(VK_FALSE)
            .setCompareOp(vk::CompareOp::eAlways)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setMipLodBias(0.0)
            .setMinLod(0.0)
            .setMaxLod(0.0);

        auto sampler_create_result = m_device.createSampler(sampler_create_info);
        if (sampler_create_result.result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not create Texture2D Sampler! Result code: {}", vk::to_string(sampler_create_result.result));
            exit(1);
        }

        m_texture2DSampler = sampler_create_result.value;
    }

    vk::Result VulkanRenderDevice::BeginGraphicsCommandBuffer(vk::CommandBuffer graphics_cmd_buffer)
    {
        const vk::Result graph_reset_result = graphics_cmd_buffer.reset();
        if (graph_reset_result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not reset current graphics command buffer of frame {}. Result code: {}", m_current_frame, vk::to_string(graph_reset_result).c_str());
            return graph_reset_result;
        }

        const vk::CommandBufferBeginInfo cmd_buffer_begin_info{};
        return graphics_cmd_buffer.begin(cmd_buffer_begin_info);
    }

    vk::Result VulkanRenderDevice::BeginTransferCommandBuffer(vk::CommandBuffer transfer_cmd_buffer)
    {
        const vk::Result transf_reset_result = transfer_cmd_buffer.reset();
        if (transf_reset_result != vk::Result::eSuccess)
        {
            BRR_LogError("Could not reset current transfer command buffer of frame {}. Result code: {}", m_current_frame, vk::to_string(transf_reset_result).c_str());
            return transf_reset_result;
        }

        const vk::CommandBufferBeginInfo cmd_buffer_begin_info{};
        return transfer_cmd_buffer.begin(cmd_buffer_begin_info);
    }

    vk::Result VulkanRenderDevice::SubmitGraphicsCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                                uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
                                                                vk::PipelineStageFlags* wait_dst_stages,
                                                                uint32_t signal_semaphore_count, vk::Semaphore* signal_semaphores,
                                                                vk::Fence submit_fence)
    {
        vk::SubmitInfo submit_info{};
        submit_info
            .setPCommandBuffers(cmd_buffers)
            .setCommandBufferCount(cmd_buffer_count)
            .setPWaitSemaphores(wait_semaphores)
            .setWaitSemaphoreCount(wait_semaphore_count)
            .setPWaitDstStageMask(wait_dst_stages)
            .setPSignalSemaphores(signal_semaphores)
            .setSignalSemaphoreCount(signal_semaphore_count);

        /*vk::CommandBufferSubmitInfo cmd_buffer_submit_info {cmd_buffer};

        vk::SemaphoreSubmitInfo image_available_semaphore_info {};
        image_available_semaphore_info
            .setDeviceIndex(0)
            .setSemaphore(m_current_image_available_semaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::SemaphoreSubmitInfo render_finished_semaphore_info {};
        render_finished_semaphore_info
            .setDeviceIndex(0)
            .setSemaphore(current_render_finished_semaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::SubmitInfo2 submit_info2 {};
        submit_info2
            .setCommandBufferInfos(cmd_buffer_submit_info)
            .setWaitSemaphoreInfos(image_available_semaphore_info)
            .setSignalSemaphoreInfos(render_finished_semaphore_info);*/

        return GetGraphicsQueue().submit(submit_info, submit_fence);
    }

    vk::Result VulkanRenderDevice::SubmitPresentCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                               uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
                                                               vk::PipelineStageFlags* wait_dst_stages,
                                                               uint32_t signal_semaphore_count, vk::Semaphore* signal_semaphores,
                                                               vk::Fence submit_fence)
    {
        //TODO
        return vk::Result::eErrorUnknown;
    }

    vk::Result VulkanRenderDevice::SubmitTransferCommandBuffers(uint32_t cmd_buffer_count, vk::CommandBuffer* cmd_buffers,
                                                                uint32_t wait_semaphore_count, vk::Semaphore* wait_semaphores,
                                                                vk::PipelineStageFlags* wait_dst_stages,
                                                                uint32_t signal_semaphore_count, vk::Semaphore* signal_semaphores,
                                                                vk::Fence submit_fence)
    {
        vk::SubmitInfo submit_info{};
        submit_info
            .setPCommandBuffers(cmd_buffers)
            .setCommandBufferCount(cmd_buffer_count)
            .setPWaitSemaphores(wait_semaphores)
            .setWaitSemaphoreCount(wait_semaphore_count)
            .setPWaitDstStageMask(wait_dst_stages)
            .setPSignalSemaphores(signal_semaphores)
            .setSignalSemaphoreCount(signal_semaphore_count);

        return GetTransferQueue().submit(submit_info, submit_fence);
    }

    void VulkanRenderDevice::Free_FramePendingResources(Frame& frame)
    {
        for (auto& buffer_alloc_pair : frame.buffer_delete_list)
        {
            vmaDestroyBuffer(m_vma_allocator, buffer_alloc_pair.first, buffer_alloc_pair.second);
        }
        frame.buffer_delete_list.clear();

        for (auto& texture_alloc_info : frame.texture_delete_list)
        {
            m_device.destroyImageView(texture_alloc_info.image_view);
            vmaDestroyImage(m_vma_allocator, texture_alloc_info.image, texture_alloc_info.allocation);
        }
        frame.texture_delete_list.clear();
    }
}
