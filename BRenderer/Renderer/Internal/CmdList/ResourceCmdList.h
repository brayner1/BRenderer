#ifndef BRR_RESOURCECMDLIST_H
#define BRR_RESOURCECMDLIST_H

#include <Renderer/RenderEnums.h>
#include <Renderer/RenderingResourceIDs.h>

namespace brr::render::internal
{
    enum class ResourceCommandType
    {
        CreateTexture2D,
        DestroyTexture2D,
        CreateSurface,
        DestroySurface
    };

    struct ResourceCommand
    {
        static ResourceCommand BuildCreateTexture2DCommand(TextureID texture_id,
                                                           const void* data,
                                                           uint32_t width,
                                                           uint32_t height,
                                                           DataFormat image_format)
        {
            size_t format_size    = GetDataFormatByteSize(image_format);
            void* image_data_copy = nullptr;
            if (data)
            {
                size_t total_size = width * height * format_size;
                image_data_copy   = malloc(total_size);
                memcpy(image_data_copy, data, total_size);
            }

            ResourceCommand resource_command;
            resource_command.command_type    = ResourceCommandType::CreateTexture2D;
            resource_command.texture_command = TextureCommand(texture_id, image_data_copy, width, height,
                                                              image_format);
            return resource_command;
        }

        static ResourceCommand BuildDestroyTexture2DCommand(TextureID texture_id)
        {
            ResourceCommand resource_command;
            resource_command.command_type = ResourceCommandType::DestroyTexture2D;
            resource_command.texture_command = TextureCommand(texture_id);
            return resource_command;
        }

        static ResourceCommand BuildCreateSurfaceCommand(SurfaceID surface_id,
                                                         const void* vertex_buffer,
                                                         size_t vertex_buffer_size,
                                                         const void* index_buffer,
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

        struct TextureCommand
        {
            TextureCommand(TextureID texture_id,
                           void* data = nullptr,
                           uint32_t width = 0,
                           uint32_t height = 0,
                           DataFormat image_format = DataFormat::Undefined)
            :  texture_id(texture_id),
               image_data(data),
               width(width),
               height(height),
               image_format(image_format)
            {}

            TextureID texture_id;
            void* image_data;
            uint32_t width, height;
            DataFormat image_format;
        };

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
            TextureCommand texture_command;
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
            case ResourceCommandType::DestroySurface:
                this->surface_command = other.surface_command;
                break;
            case ResourceCommandType::CreateTexture2D:
            case ResourceCommandType::DestroyTexture2D:
                this->texture_command = other.texture_command;
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