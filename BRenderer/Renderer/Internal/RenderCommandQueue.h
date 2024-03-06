#ifndef BRR_RENDERCMDQUEUE_H
#define BRR_RENDERCMDQUEUE_H

#include <Renderer/ResourcesHandles.h>

namespace brr::render
{
    class RenderCommandQueue
    {
    public:

        RenderCommandQueue();

        void Command_BufferCopy(BufferHandle dst_buffer_handle, void* src_data, size_t size);

        void Command_ImageCopy(Texture2DHandle dst_image_handle, void* src_data, size_t size);


    };
}

#endif