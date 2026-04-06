#include "UVLayoutGenerator.h"
#include "DecalBakerLog.h"
#include "UVOverlapDetector.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"

#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

FMeshUVStatus FUVLayoutGenerator::ResolveUVChannel(
    UStaticMesh* StaticMesh,
    EDecalBakerUVStrategy Strategy,
    int32 UVPadding)
{
    FMeshUVStatus Status;
    if (!StaticMesh) return Status;

    switch (Strategy)
    {
    case EDecalBakerUVStrategy::ForceUV0:
        Status.BakeUVChannel = 0;
        Status.bHasOverlap = FUVOverlapDetector::AnalyzeMeshUVs(StaticMesh, 0).bHasOverlap;
        if (Status.bHasOverlap)
        {
            UE_LOG(LogDecalBaker, Warning, TEXT("DecalBaker: UV0 has overlaps on %s, bake may produce artifacts"),
                *StaticMesh->GetName());
        }
        break;

    case EDecalBakerUVStrategy::ForceUV1:
        Status.BakeUVChannel = 1;
        Status.bHasOverlap = FUVOverlapDetector::AnalyzeMeshUVs(StaticMesh, 1).bHasOverlap;
        if (Status.bHasOverlap)
        {
            UE_LOG(LogDecalBaker, Warning, TEXT("DecalBaker: UV1 has overlaps on %s, bake may produce artifacts"),
                *StaticMesh->GetName());
        }
        break;

    case EDecalBakerUVStrategy::ForceGenerate:
    {
        int32 NewChannel = GenerateNonOverlappingUVs(StaticMesh, UVPadding);
        if (NewChannel >= 0)
        {
            Status.BakeUVChannel = NewChannel;
            Status.bGeneratedUV = true;
        }
        else
        {
            UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: Failed to generate UVs for %s"),
                *StaticMesh->GetName());
        }
        break;
    }

    case EDecalBakerUVStrategy::Auto:
    default:
    {
        FMeshUVStatus UV0Status = FUVOverlapDetector::AnalyzeMeshUVs(StaticMesh, 0);
        if (!UV0Status.bHasOverlap)
        {
            Status.BakeUVChannel = 0;
            break;
        }

        if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
        {
            Status.BakeUVChannel = 0;
            break;
        }
        const FStaticMeshLODResources& LOD = StaticMesh->GetRenderData()->LODResources[0];
        int32 NumUVChannels = LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
        if (NumUVChannels > 1)
        {
            FMeshUVStatus UV1Status = FUVOverlapDetector::AnalyzeMeshUVs(StaticMesh, 1);
            if (!UV1Status.bHasOverlap)
            {
                Status.BakeUVChannel = 1;
                break;
            }
        }

        int32 NewChannel = GenerateNonOverlappingUVs(StaticMesh, UVPadding);
        if (NewChannel >= 0)
        {
            Status.BakeUVChannel = NewChannel;
            Status.bGeneratedUV = true;
            break;
        }

        UE_LOG(LogDecalBaker, Warning,
            TEXT("DecalBaker: All UV strategies exhausted for %s, falling back to UV0 with potential artifacts"),
            *StaticMesh->GetName());
        Status.BakeUVChannel = 0;
        Status.bHasOverlap = true;
        break;
    }
    }

    return Status;
}

int32 FUVLayoutGenerator::GenerateNonOverlappingUVs(
    UStaticMesh* StaticMesh,
    int32 UVPadding)
{
#if !WITH_EDITOR
    UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: UV generation requires an editor build"));
    return -1;
#else
    if (!StaticMesh) return -1;

    FMeshDescription* MeshDesc = StaticMesh->GetMeshDescription(0);
    if (!MeshDesc) return -1;

    FStaticMeshAttributes Attributes(*MeshDesc);
    int32 ExistingChannels = Attributes.GetVertexInstanceUVs().GetNumChannels();

    int32 NewChannel = ExistingChannels;
    if (NewChannel >= MAX_STATIC_TEXCOORDS)
    {
        UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: Cannot add UV channel - max %d reached on %s"),
            MAX_STATIC_TEXCOORDS, *StaticMesh->GetName());
        return -1;
    }

    StaticMesh->Modify();

    FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel(0);
    SourceModel.BuildSettings.SrcLightmapIndex = 0;
    SourceModel.BuildSettings.DstLightmapIndex = NewChannel;
    SourceModel.BuildSettings.MinLightmapResolution = FMath::Max(64, UVPadding * 16);
    SourceModel.BuildSettings.bGenerateLightmapUVs = true;

    StaticMesh->Build(false);
    StaticMesh->PostEditChange();

    UE_LOG(LogDecalBaker, Log, TEXT("DecalBaker: Generated non-overlapping UVs at channel %d for %s"),
        NewChannel, *StaticMesh->GetName());

    return NewChannel;
#endif
}
