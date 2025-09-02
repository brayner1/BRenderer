#include "MeshStorage.h"

#include <Renderer/SceneObjectsIDs.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>

#include "RenderStorageGlobals.h"

using namespace brr;
using namespace brr::render;

void DestroySurfaceBuffers(RenderSurface& surface)
{
    VKRD* render_device = VKRD::GetSingleton();
    if (surface.m_vertex_buffer)
        render_device->DestroyVertexBuffer(surface.m_vertex_buffer);

    if (surface.m_index_buffer)
        render_device->DestroyIndexBuffer(surface.m_index_buffer);
}

MeshStorage::~MeshStorage()
{
    //for (RenderSurface& surface : m_surfaces_allocator)
    //{
    //    DestroySurfaceBuffers(surface);
    //}
}

void MeshStorage::InitSurface(SurfaceID surface_id,
                              void* vertex_buffer_data,
                              size_t vertex_buffer_size,
                              void* index_buffer_data,
                              size_t index_buffer_size,
                              MaterialID surface_material)
{
    RenderSurface* surface = InitResource(surface_id, RenderSurface());

    if (!surface)
    {
        BRR_LogError("Trying to initialize Surface (ID: {}) that does not exist.", static_cast<uint64_t>(surface_id));
        return;
    }

    if (!vertex_buffer_data || vertex_buffer_size == 0)
    {
        BRR_LogError("Vertex Buffer data can't be null or empty.");
        return;
    }

    BRR_LogDebug("Initializing new RenderSurface (ID: {}).", static_cast<uint64_t>(surface_id));

    BRR_LogInfo("Creating Vertex Buffer.");

    //TODO: vertex format should be either: 1. Enforced; 2. Passed as parameter;
    VulkanRenderDevice::VertexFormatFlags vertex_format = VKRD::VertexFormatFlags::UV0 | 
        VKRD::VertexFormatFlags::NORMAL |
        VKRD::VertexFormatFlags::TANGENT;

    surface->m_vertex_buffer = VKRD::GetSingleton()->CreateVertexBuffer(
        vertex_buffer_size, vertex_format, vertex_buffer_data);

    surface->num_vertices = vertex_buffer_size / sizeof(Vertex3);

    if (!index_buffer_data || index_buffer_size == 0)
            return;

    BRR_LogInfo("Creating Index Buffer.");

    VulkanRenderDevice::IndexType index_type = VulkanRenderDevice::IndexType::UINT32;

    surface->m_index_buffer = VKRD::GetSingleton()->CreateIndexBuffer(
        index_buffer_size, index_type, index_buffer_data);

    surface->num_indices = index_buffer_size / sizeof(uint32_t);

    if (surface_material.IsValid() && RenderStorageGlobals::material_storage.GetMaterial(surface_material) != nullptr)
    {
        surface->m_material_id = surface_material;
    }
    else
    {
        BRR_LogError("Invalid Material ID for Surface (ID: {}).", static_cast<uint64_t>(surface_id));
    }
}

void MeshStorage::DestroySurface(SurfaceID surface_id)
{
    if (RenderSurface* surface = GetResource(surface_id))
    {
        DestroySurfaceBuffers(*surface);
        DestroyResource(surface_id);
    }
}

void MeshStorage::UpdateSurfaceVertexBuffer(SurfaceID surface_id,
                                            void* vertex_buffer_data,
                                            size_t vertex_buffer_size)
{
    //TODO: Implement vertex buffer update (need to avoid buffer being updated at the same time it's used by previous frame)
}

void MeshStorage::UpdateSurfaceIndexBuffer(SurfaceID surface_id,
                                           void* index_buffer_data,
                                           size_t index_buffer_size)
{
    //TODO: Implement index buffer update (need to avoid buffer being updated at the same time it's used by previous frame)
}

RenderSurface* MeshStorage::GetSurface(SurfaceID surface_id)
{
    return GetResource(surface_id);
}
