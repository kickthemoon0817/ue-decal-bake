#pragma once

#include "CoreMinimal.h"
#include "DecalBakerTypes.h"

class UStaticMesh;

class DECALBAKERRUNTIME_API FUVLayoutGenerator
{
public:
    static FMeshUVStatus ResolveUVChannel(
        UStaticMesh* StaticMesh,
        EDecalBakerUVStrategy Strategy,
        int32 UVPadding = 4);

private:
    static int32 GenerateNonOverlappingUVs(
        UStaticMesh* StaticMesh,
        int32 UVPadding);
};
