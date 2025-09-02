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
    case ResourceCommandType::CreateTexture2D:
        {
            BRR_LogDebug("RenderThread: Creating Texture (ID: {})", static_cast<size_t>(resource_command.texture_command.texture_id));
            RenderStorageGlobals::texture_storage.InitTexture(resource_command.texture_command.texture_id,
                                                              resource_command.texture_command.image_data,
                                                              resource_command.texture_command.width,
                                                              resource_command.texture_command.height,
                                                              resource_command.texture_command.image_format);
            if (resource_command.texture_command.image_data)
            {
                free(resource_command.texture_command.image_data);
            }
            break;
        }
    case ResourceCommandType::DestroyTexture2D:
        {
            BRR_LogDebug("RenderThread: Destroying Texture (ID: {})", static_cast<size_t>(resource_command.texture_command.texture_id));
            RenderStorageGlobals::texture_storage.DestroyTexture(resource_command.texture_command.texture_id);
            break;
        }
    case ResourceCommandType::CreateMaterial:
        {

            BRR_LogDebug("RenderThread: Creating Material (ID: {})", static_cast<size_t>(resource_command.material_command.material_id));
            RenderStorageGlobals::material_storage.InitMaterial(resource_command.material_command.material_id,
                                                               resource_command.material_command.material_properties);
            break;
        }
    case ResourceCommandType::UpdateMaterial:
        {
            BRR_LogDebug("RenderThread: Updating Material properties (ID: {})", static_cast<size_t>(resource_command.material_command.material_id));
            RenderStorageGlobals::material_storage.UpdateMaterialProperties(resource_command.material_command.material_id,
                                                                  resource_command.material_command.material_properties);
            break;
        }
    case ResourceCommandType::DestroyMaterial:
        {
            BRR_LogDebug("RenderThread: Destroying Material (ID: {})", static_cast<size_t>(resource_command.material_command.material_id));
            RenderStorageGlobals::material_storage.DestroyMaterial(resource_command.material_command.material_id);
            break;
        }
    case ResourceCommandType::CreateSurface:
        {
            BRR_LogDebug("RenderThread: Creating Surface (ID: {})", static_cast<size_t>(resource_command.surface_command.surface_id));
            RenderStorageGlobals::mesh_storage.InitSurface(resource_command.surface_command.surface_id,
                                                           resource_command.surface_command.vertex_buffer,
                                                           resource_command.surface_command.vertex_buffer_size,
                                                           resource_command.surface_command.index_buffer,
                                                           resource_command.surface_command.index_buffer_size, 
                                                           resource_command.surface_command.material_id);
            if (resource_command.surface_command.vertex_buffer)
            {
                free(resource_command.surface_command.vertex_buffer);
            }
            if (resource_command.surface_command.index_buffer)
            {
                free(resource_command.surface_command.index_buffer);
            }
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
