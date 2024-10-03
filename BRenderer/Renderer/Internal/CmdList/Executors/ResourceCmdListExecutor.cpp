#include "ResourceCmdListExecutor.h"

#include <Renderer/Allocators/SystemsOwner.h>
#include <Renderer/Storages/RenderStorageGlobals.h>

using namespace brr::render::internal;

void ResourceCmdListExecutor::ExecuteCmdList()
{
    for (auto& resource_cmd : m_resource_cmd_list)
    {
        ExecuteResourceCommand(resource_cmd);
    }
}

void ResourceCmdListExecutor::ExecuteResourceCommand(const ResourceCommand& resource_command)
{
    switch (resource_command.command_type)
    {
    case ResourceCommandType::CreateSurface:
        {
            BRR_LogDebug("RenderThread: Creating Surface (ID: {})", static_cast<size_t>(resource_command.surface_command.surface_id));
            RenderStorageGlobals::mesh_storage.InitSurface(resource_command.surface_command.surface_id,
                                                           resource_command.surface_command.vertex_buffer,
                                                           resource_command.surface_command.vertex_buffer_size,
                                                           resource_command.surface_command.index_buffer,
                                                           resource_command.surface_command.index_buffer_size);
            free(resource_command.surface_command.vertex_buffer);
            free(resource_command.surface_command.index_buffer);
            break;
        }
    case ResourceCommandType::DestroySurface:
        {
            BRR_LogDebug("RenderThread: Destroying Surface (ID: {})", static_cast<size_t>(resource_command.surface_command.surface_id));
            RenderStorageGlobals::mesh_storage.DestroySurface(resource_command.surface_command.surface_id);

            for (auto& scene_renderers_it : SystemsStorage::GetSceneRendererStorage())
            {
                scene_renderers_it.second->NotifySurfaceChanged(resource_command.surface_command.surface_id);
            }
            break;
        }
    }
}
