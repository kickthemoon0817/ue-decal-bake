#include "DecalBakerSubsystem.h"
#include "DecalProjection.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void UDecalBakerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UDecalBakerSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

TArray<FDecalMeshPair> UDecalBakerSubsystem::DiscoverDecalMeshPairs(
    UWorld* World,
    const TArray<UStaticMeshComponent*>& InScope)
{
    TArray<FDecalMeshPair> Pairs;
    if (!World) return Pairs;

    TArray<UDecalComponent*> Decals = FindAllDecals(World);

    for (UDecalComponent* Decal : Decals)
    {
        FMatrix ProjMatrix = FDecalProjection::ComputeProjectionMatrix(Decal);
        TArray<UStaticMeshComponent*> AffectedMeshes = FindAffectedMeshes(World, Decal, InScope);

        for (UStaticMeshComponent* Mesh : AffectedMeshes)
        {
            FDecalMeshPair Pair;
            Pair.DecalComponent = Decal;
            Pair.MeshComponent = Mesh;
            Pair.ProjectionMatrix = ProjMatrix;
            Pair.SortOrder = Decal->SortOrder;
            Pairs.Add(Pair);
        }
    }

    Pairs.Sort([](const FDecalMeshPair& A, const FDecalMeshPair& B)
    {
        if (A.MeshComponent.Get() != B.MeshComponent.Get())
        {
            return A.MeshComponent.Get() < B.MeshComponent.Get();
        }
        return A.SortOrder < B.SortOrder;
    });

    return Pairs;
}

FDecalBakeManifest UDecalBakerSubsystem::BakeDecals(
    UWorld* World,
    const TArray<UStaticMeshComponent*>& InScope)
{
    FDecalBakeManifest Manifest;
    // Full implementation in Tasks 7-8
    return Manifest;
}

void UDecalBakerSubsystem::RevertBake(const FDecalBakeManifest& Manifest)
{
    // Full implementation in Task 8
}

TArray<UDecalComponent*> UDecalBakerSubsystem::FindAllDecals(UWorld* World) const
{
    TArray<UDecalComponent*> Decals;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        TArray<UDecalComponent*> Components;
        It->GetComponents<UDecalComponent>(Components);
        Decals.Append(Components);
    }
    return Decals;
}

TArray<UStaticMeshComponent*> UDecalBakerSubsystem::FindAffectedMeshes(
    UWorld* World,
    UDecalComponent* Decal,
    const TArray<UStaticMeshComponent*>& InScope) const
{
    TArray<UStaticMeshComponent*> Result;

    FBox DecalBox(-Decal->DecalSize, Decal->DecalSize);
    FTransform DecalTransform = Decal->GetComponentTransform();

    TArray<UStaticMeshComponent*> Candidates;
    if (InScope.Num() > 0)
    {
        Candidates = InScope;
    }
    else
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            TArray<UStaticMeshComponent*> Components;
            It->GetComponents<UStaticMeshComponent>(Components);
            Candidates.Append(Components);
        }
    }

    for (UStaticMeshComponent* Mesh : Candidates)
    {
        if (!Mesh || !Mesh->GetStaticMesh()) continue;
        if (!Mesh->bReceivesDecals) continue;

        FBoxSphereBounds MeshBounds = Mesh->Bounds;
        FBox MeshBox = MeshBounds.GetBox();
        FBox MeshInDecalSpace = MeshBox.TransformBy(DecalTransform.Inverse());
        if (DecalBox.Intersect(MeshInDecalSpace))
        {
            Result.Add(Mesh);
        }
    }

    return Result;
}
