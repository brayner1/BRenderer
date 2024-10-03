#ifndef BRR_RESOURCECMDLIST_H
#define BRR_RESOURCECMDLIST_H

#include <Renderer/SceneResourcesHandles.h>

namespace brr::render::internal
{
    enum class ResourceCommandType
    {
        CreateSurface,
        DestroySurface
    };

    struct ResourceCommand
    {
        static ResourceCommand BuildCreateSurfaceCommand(SurfaceID surface_id,
                                                         void* vertex_buffer,
                                                         size_t vertex_buffer_size,
                                                         void* index_buffer,
                                                         size_t index_buffer_size)
        {
            void* vertex_buffer_copy = nullptr;
            if (vertex_buffer_size > 0)
            {
                vertex_buffer_copy = malloc(vertex_buffer_size);
                memcpy(vertex_buffer_copy, vertex_buffer, vertex_buffer_size);
            }

            void* index_buffer_copy = nullptr;
            if (index_buffer_size > 0)
            {
                index_buffer_copy = malloc(index_buffer_size);
                memcpy(index_buffer_copy, index_buffer, index_buffer_size);
            }

            ResourceCommand resource_command;
            resource_command.command_type    = ResourceCommandType::CreateSurface;
            resource_command.surface_command = SurfaceCommand(surface_id,
                                                              vertex_buffer_copy,
                                                              vertex_buffer_size,
                                                              index_buffer_copy,
                                                              index_buffer_size);

            return resource_command;
        }

        static ResourceCommand BuildDestroySurfaceCommand(SurfaceID surface_id)
        {
            ResourceCommand resource_command;
            resource_command.command_type = ResourceCommandType::DestroySurface;
            resource_command.surface_command = SurfaceCommand(surface_id);
            return resource_command;
        }

    public:
        ResourceCommandType command_type;

        struct SurfaceCommand
        {
            SurfaceCommand(SurfaceID surface_id,
                           void* vertex_buffer = nullptr,
                           size_t vertex_buffer_size = 0,
                           void* index_buffer = nullptr,
                           size_t index_buffer_size = 0)
                : surface_id(surface_id),
                  vertex_buffer(vertex_buffer),
                  vertex_buffer_size(vertex_buffer_size),
                  index_buffer(index_buffer),
                  index_buffer_size(index_buffer_size)
            {
            }

            SurfaceID surface_id;
            void* vertex_buffer;
            size_t vertex_buffer_size;
            void* index_buffer;
            size_t index_buffer_size;
        };

        union
        {
            SurfaceCommand surface_command;

            struct
            {

            } texture_command;
        };

        ResourceCommand(const ResourceCommand& other)
        {
            *this = other;
        }

        ResourceCommand& operator=(const ResourceCommand& other)
        {
            this->command_type = other.command_type;
            switch (command_type) {
            case ResourceCommandType::CreateSurface:
                this->surface_command = other.surface_command;

                break;
            case ResourceCommandType::DestroySurface:
                this->surface_command = other.surface_command;
                break;
            }

            return *this;
        }

    private:
        ResourceCommand()
            : command_type()
        {
        }

    };

    using ResourceCmdList = CmdList<ResourceCommand>;
}

#endif