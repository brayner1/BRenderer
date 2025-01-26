#ifndef BRR_MESHSTORAGE_H
#define BRR_MESHSTORAGE_H

#include <Core/Storage/ResourceAllocator.h>
#include <Geometry/Geometry.h>
#include <Renderer/GpuResources/GpuResourcesHandles.h>
#include <Renderer/RenderingResourceIDs.h>

namespace brr::render
{
    class MeshStorage
    {
    public:

        struct RenderSurface
        {
            VertexBufferHandle m_vertex_buffer;
            IndexBufferHandle m_index_buffer;

            uint32_t num_vertices = 0, num_indices = 0;

            AABBB m_aabb;
        };

        MeshStorage() = default;

        ~MeshStorage();

        SurfaceID AllocateSurface();

        void InitSurface(SurfaceID surface_id,
                         void* vertex_buffer_data,
                         size_t vertex_buffer_size,
                         void* index_buffer_data,
                         size_t index_buffer_size);

        void DestroySurface(SurfaceID surface_id);

        void UpdateSurfaceVertexBuffer(SurfaceID surface_id,
                                       void* vertex_buffer_data,
                                       size_t vertex_buffer_size);

        void UpdateSurfaceIndexBuffer(SurfaceID surface_id,
                                      void* index_buffer_data,
                                      size_t index_buffer_size);

        RenderSurface* GetSurface(SurfaceID surface_id);

        

    private:

        ResourceAllocator<RenderSurface> m_surfaces_allocator;
    };
}

#endif