#include "UVOverlapDetector.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"

bool FUVOverlapDetector::HasOverlappingTriangles(
    const TArray<FVector2D>& UVs,
    const TArray<uint32>& Indices)
{
    const int32 NumTris = Indices.Num() / 3;
    const int32 GridSize = FMath::Max(8, FMath::CeilToInt(FMath::Sqrt((float)NumTris)));

    struct FGridCell { TArray<int32> TriIndices; };
    TArray<FGridCell> Grid;
    Grid.SetNum(GridSize * GridSize);

    for (int32 TriIdx = 0; TriIdx < NumTris; ++TriIdx)
    {
        const FVector2D& V0 = UVs[Indices[TriIdx * 3 + 0]];
        const FVector2D& V1 = UVs[Indices[TriIdx * 3 + 1]];
        const FVector2D& V2 = UVs[Indices[TriIdx * 3 + 2]];

        float MinX = FMath::Min3(V0.X, V1.X, V2.X);
        float MaxX = FMath::Max3(V0.X, V1.X, V2.X);
        float MinY = FMath::Min3(V0.Y, V1.Y, V2.Y);
        float MaxY = FMath::Max3(V0.Y, V1.Y, V2.Y);

        int32 CellMinX = FMath::Clamp(FMath::FloorToInt(MinX * GridSize), 0, GridSize - 1);
        int32 CellMaxX = FMath::Clamp(FMath::FloorToInt(MaxX * GridSize), 0, GridSize - 1);
        int32 CellMinY = FMath::Clamp(FMath::FloorToInt(MinY * GridSize), 0, GridSize - 1);
        int32 CellMaxY = FMath::Clamp(FMath::FloorToInt(MaxY * GridSize), 0, GridSize - 1);

        for (int32 Y = CellMinY; Y <= CellMaxY; ++Y)
        {
            for (int32 X = CellMinX; X <= CellMaxX; ++X)
            {
                Grid[Y * GridSize + X].TriIndices.Add(TriIdx);
            }
        }
    }

    TSet<uint64> TestedPairs;
    for (const FGridCell& Cell : Grid)
    {
        for (int32 i = 0; i < Cell.TriIndices.Num(); ++i)
        {
            for (int32 j = i + 1; j < Cell.TriIndices.Num(); ++j)
            {
                int32 TriA = Cell.TriIndices[i];
                int32 TriB = Cell.TriIndices[j];

                uint64 PairKey = ((uint64)FMath::Min(TriA, TriB) << 32) | (uint64)FMath::Max(TriA, TriB);
                if (TestedPairs.Contains(PairKey)) continue;
                TestedPairs.Add(PairKey);

                const FVector2D& A0 = UVs[Indices[TriA * 3 + 0]];
                const FVector2D& A1 = UVs[Indices[TriA * 3 + 1]];
                const FVector2D& A2 = UVs[Indices[TriA * 3 + 2]];
                const FVector2D& B0 = UVs[Indices[TriB * 3 + 0]];
                const FVector2D& B1 = UVs[Indices[TriB * 3 + 1]];
                const FVector2D& B2 = UVs[Indices[TriB * 3 + 2]];

                if (SharesEdge(A0, A1, A2, B0, B1, B2)) continue;

                if (TrianglesOverlap2D(A0, A1, A2, B0, B1, B2))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

FMeshUVStatus FUVOverlapDetector::AnalyzeMeshUVs(
    const UStaticMesh* StaticMesh,
    int32 UVChannel)
{
    FMeshUVStatus Status;
    if (!StaticMesh || !StaticMesh->GetRenderData()) return Status;
    if (StaticMesh->GetRenderData()->LODResources.Num() == 0) return Status;

    const FStaticMeshLODResources& LOD = StaticMesh->GetRenderData()->LODResources[0];
    const int32 NumUVChannels = LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();

    if (UVChannel >= NumUVChannels)
    {
        Status.bHasOverlap = true;
        return Status;
    }

    TArray<FVector2D> UVs;
    const int32 NumVertices = LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices();
    UVs.Reserve(NumVertices);
    for (int32 i = 0; i < NumVertices; ++i)
    {
        UVs.Add(FVector2D(LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, UVChannel)));
    }

    TArray<uint32> Indices;
    LOD.IndexBuffer.GetCopy(Indices);

    Status.bHasOverlap = HasOverlappingTriangles(UVs, Indices);
    Status.BakeUVChannel = UVChannel;

    return Status;
}

bool FUVOverlapDetector::TrianglesOverlap2D(
    const FVector2D& A0, const FVector2D& A1, const FVector2D& A2,
    const FVector2D& B0, const FVector2D& B1, const FVector2D& B2)
{
    if (PointInTriangle2D(A0, B0, B1, B2)) return true;
    if (PointInTriangle2D(A1, B0, B1, B2)) return true;
    if (PointInTriangle2D(A2, B0, B1, B2)) return true;
    if (PointInTriangle2D(B0, A0, A1, A2)) return true;
    if (PointInTriangle2D(B1, A0, A1, A2)) return true;
    if (PointInTriangle2D(B2, A0, A1, A2)) return true;

    FVector2D EdgesA[3][2] = { {A0, A1}, {A1, A2}, {A2, A0} };
    FVector2D EdgesB[3][2] = { {B0, B1}, {B1, B2}, {B2, B0} };

    for (int32 i = 0; i < 3; ++i)
    {
        for (int32 j = 0; j < 3; ++j)
        {
            if (SegmentsIntersect2D(EdgesA[i][0], EdgesA[i][1], EdgesB[j][0], EdgesB[j][1]))
            {
                return true;
            }
        }
    }

    return false;
}

bool FUVOverlapDetector::SharesEdge(
    const FVector2D& A0, const FVector2D& A1, const FVector2D& A2,
    const FVector2D& B0, const FVector2D& B1, const FVector2D& B2)
{
    const float Tolerance = SMALL_NUMBER;
    FVector2D VertsA[3] = { A0, A1, A2 };
    FVector2D VertsB[3] = { B0, B1, B2 };

    int32 SharedCount = 0;
    for (int32 i = 0; i < 3; ++i)
    {
        for (int32 j = 0; j < 3; ++j)
        {
            if (FVector2D::DistSquared(VertsA[i], VertsB[j]) < Tolerance)
            {
                SharedCount++;
                break;
            }
        }
    }
    return SharedCount >= 2;
}

bool FUVOverlapDetector::PointInTriangle2D(
    const FVector2D& P,
    const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
    float d1 = (P.X - B.X) * (A.Y - B.Y) - (A.X - B.X) * (P.Y - B.Y);
    float d2 = (P.X - C.X) * (B.Y - C.Y) - (B.X - C.X) * (P.Y - C.Y);
    float d3 = (P.X - A.X) * (C.Y - A.Y) - (C.X - A.X) * (P.Y - A.Y);

    bool HasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool HasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(HasNeg && HasPos);
}

bool FUVOverlapDetector::SegmentsIntersect2D(
    const FVector2D& A1, const FVector2D& A2,
    const FVector2D& B1, const FVector2D& B2)
{
    FVector2D D1 = A2 - A1;
    FVector2D D2 = B2 - B1;

    float Cross = D1.X * D2.Y - D1.Y * D2.X;
    if (FMath::IsNearlyZero(Cross)) return false;

    FVector2D D = B1 - A1;
    float T = (D.X * D2.Y - D.Y * D2.X) / Cross;
    float U = (D.X * D1.Y - D.Y * D1.X) / Cross;

    const float Eps = KINDA_SMALL_NUMBER;
    return T > Eps && T < (1.0f - Eps) && U > Eps && U < (1.0f - Eps);
}
