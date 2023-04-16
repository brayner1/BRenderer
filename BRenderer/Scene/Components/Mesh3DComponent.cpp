#include "Scene/Components/Mesh3DComponent.h"

#include "Renderer/Renderer.h"

namespace brr
{
    Mesh3DComponent::SurfaceData::SurfaceData()
    : m_surfaceId(currentSurfaceID++)
    {

    }

    Mesh3DComponent::SurfaceData::SurfaceData(const SurfaceData& surface)
    : m_vertices(surface.m_vertices),
      m_indices(surface.m_indices),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
        SDL_Log("Mesh3DComponent Initialized.");
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor>&& vertices)
    : m_vertices(std::move(vertices)),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
        SDL_Log("Mesh3DComponent Initialized.");
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor> vertices)
    : m_vertices(std::move(vertices)),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
        SDL_Log("Mesh3DComponent Initialized.");
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor>&& vertices, std::vector<uint32_t>&& indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices)),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
        SDL_Log("Mesh3DComponent Initialized.");
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor> vertices, std::vector<uint32_t> indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices)),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
        SDL_Log("Mesh3DComponent Initialized.");
    }

    //void Mesh3DComponent::SurfaceData::Bind(vk::CommandBuffer command_buffer) const
    //{
    //	assert(m_vertex_buffer.IsValid() && "Vertex buffer must be valid to bind to a command buffer.");
    //	command_buffer.bindVertexBuffers(0, m_vertex_buffer.GetBuffer(), { 0 });

    //	if (m_index_buffer.IsValid())
    //	{
    //		command_buffer.bindIndexBuffer(m_index_buffer.GetBuffer(), 0, vk::IndexType::eUint32);
    //	}

    //	//command_buffer.bindDescriptorSets(1, );
    //}

    //void Mesh3DComponent::SurfaceData::Draw(vk::CommandBuffer command_buffer) const
    //{
    //	assert(m_vertex_buffer.IsValid() && "Vertex buffer must be valid to Draw to a command buffer.");
    //	if (m_index_buffer.IsValid())
    //	{
    //		command_buffer.drawIndexed(m_indices.size(), 1, 0, 0, 0);
    //	}
    //	else
    //	{
    //		command_buffer.draw(m_vertices.size(), 1, 0, 0);
    //	}
    //}

}
