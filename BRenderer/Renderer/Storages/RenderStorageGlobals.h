#ifndef BRR_RENDERSTORAGEGLOBALS_H
#define BRR_RENDERSTORAGEGLOBALS_H

#include "MeshStorage.h"
#include "TextureStorage.h"

namespace brr::render
{
    class RenderStorageGlobals
    {
    public:

        static MeshStorage mesh_storage;
        static TextureStorage texture_storage;
    };
}

#endif