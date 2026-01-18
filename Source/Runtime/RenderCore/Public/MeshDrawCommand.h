#pragma once

#include "Platform.h"

namespace Thunder
{
    struct MeshDrawCommandInfo
    {
        uint64 CommandIndex;
    };

    struct CachedPassMeshDrawList
    {
        TMap<uint64, struct RHICachedDrawCommand*> MeshDrawCommands;
    };
}
