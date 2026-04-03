#pragma once

#include "CoreMinimal.h"

class UDecalComponent;

class DECALBAKERRUNTIME_API FDecalProjection
{
public:
    static FMatrix ComputeProjectionMatrix(const FTransform& DecalTransform, const FVector& DecalSize);
    static FMatrix ComputeProjectionMatrix(const UDecalComponent* DecalComponent);
    static bool IsPointInsideDecal(const FVector& WorldPoint, const FMatrix& ProjectionMatrix);
    static FVector GetDecalForwardDir(const FTransform& DecalTransform);
};
