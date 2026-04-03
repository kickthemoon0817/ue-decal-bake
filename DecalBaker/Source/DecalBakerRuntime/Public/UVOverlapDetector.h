#pragma once

#include "CoreMinimal.h"
#include "DecalBakerTypes.h"

class UStaticMesh;

class DECALBAKERRUNTIME_API FUVOverlapDetector
{
public:
    static bool HasOverlappingTriangles(
        const TArray<FVector2D>& UVs,
        const TArray<uint32>& Indices);

    static FMeshUVStatus AnalyzeMeshUVs(
        const UStaticMesh* StaticMesh,
        int32 UVChannel = 0);

private:
    static bool TrianglesOverlap2D(
        const FVector2D& A0, const FVector2D& A1, const FVector2D& A2,
        const FVector2D& B0, const FVector2D& B1, const FVector2D& B2);

    static bool SharesEdge(
        const FVector2D& A0, const FVector2D& A1, const FVector2D& A2,
        const FVector2D& B0, const FVector2D& B1, const FVector2D& B2);

    static bool PointInTriangle2D(
        const FVector2D& P,
        const FVector2D& A, const FVector2D& B, const FVector2D& C);

    static bool SegmentsIntersect2D(
        const FVector2D& A1, const FVector2D& A2,
        const FVector2D& B1, const FVector2D& B2);
};
