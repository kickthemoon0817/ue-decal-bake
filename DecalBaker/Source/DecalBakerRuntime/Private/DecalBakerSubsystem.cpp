#include "DecalBakerSubsystem.h"
#include "DecalProjection.h"
#include "UVOverlapDetector.h"
#include "UVLayoutGenerator.h"
#include "TextureBaker.h"
#include "DecalBakerSettings.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Json.h"
#include "JsonObjectConverter.h"

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
    if (!World) return Manifest;

    const UDecalBakerSettings* Settings = GetDefault<UDecalBakerSettings>();

    // Stage 1: Discovery
    TArray<FDecalMeshPair> AllPairs = DiscoverDecalMeshPairs(World, InScope);
    if (AllPairs.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("DecalBaker: No decal-mesh pairs found"));
        return Manifest;
    }

    // Group pairs by mesh
    TMap<UStaticMeshComponent*, TArray<FDecalMeshPair>> MeshToDecals;
    for (const FDecalMeshPair& Pair : AllPairs)
    {
        UStaticMeshComponent* Mesh = Pair.MeshComponent.Get();
        if (Mesh)
        {
            MeshToDecals.FindOrAdd(Mesh).Add(Pair);
        }
    }

    for (auto& KV : MeshToDecals)
    {
        UStaticMeshComponent* MeshComp = KV.Key;
        TArray<FDecalMeshPair>& Pairs = KV.Value;
        UStaticMesh* StaticMesh = MeshComp->GetStaticMesh();
        if (!StaticMesh) continue;

        // Stage 2: UV Analysis
        FMeshUVStatus UVStatus = FUVLayoutGenerator::ResolveUVChannel(
            StaticMesh, Settings->UVStrategy, Settings->UVPadding);

        // Stage 3 + 4: GPU Bake + Texture Export
        FTextureBaker::FBakeInput BakeInput;
        BakeInput.MeshComponent = MeshComp;
        BakeInput.DecalPairs = Pairs;
        BakeInput.BakeUVChannel = UVStatus.BakeUVChannel;
        BakeInput.Resolution = Settings->OutputResolution;
        BakeInput.OutputPath = Settings->OutputPath / StaticMesh->GetName();
        BakeInput.Settings = Settings;

        FTextureBaker::FBakeOutput BakeOutput = FTextureBaker::BakeMesh(BakeInput);
        if (!BakeOutput.bSuccess) continue;

        // Stage 5: Material Assignment
        UMaterialInterface* OriginalMaterial = MeshComp->GetMaterial(0);
        FString OriginalMaterialPath = OriginalMaterial ? OriginalMaterial->GetPathName() : TEXT("");

        FString MICPath = BakeInput.OutputPath / TEXT("MI_") + StaticMesh->GetName() + TEXT("_Baked");
        FString MICPackageName = FPackageName::ObjectPathToPackageName(MICPath);
        UPackage* MICPackage = CreatePackage(*MICPackageName);

        UMaterialInstanceConstant* BakedMIC = NewObject<UMaterialInstanceConstant>(
            MICPackage,
            *FString::Printf(TEXT("MI_%s_Baked"), *StaticMesh->GetName()),
            RF_Public | RF_Standalone
        );

        if (OriginalMaterial)
        {
            BakedMIC->SetParentEditorOnly(OriginalMaterial);
        }

        if (BakeOutput.BaseColorTexture)
        {
            BakedMIC->SetTextureParameterValueEditorOnly(
                FMaterialParameterInfo(TEXT("BaseColor")), BakeOutput.BaseColorTexture);
        }
        if (BakeOutput.NormalTexture)
        {
            BakedMIC->SetTextureParameterValueEditorOnly(
                FMaterialParameterInfo(TEXT("Normal")), BakeOutput.NormalTexture);
        }
        if (BakeOutput.RoughnessTexture)
        {
            BakedMIC->SetTextureParameterValueEditorOnly(
                FMaterialParameterInfo(TEXT("Roughness")), BakeOutput.RoughnessTexture);
        }
        if (BakeOutput.MetallicTexture)
        {
            BakedMIC->SetTextureParameterValueEditorOnly(
                FMaterialParameterInfo(TEXT("Metallic")), BakeOutput.MetallicTexture);
        }
        if (BakeOutput.EmissiveTexture)
        {
            BakedMIC->SetTextureParameterValueEditorOnly(
                FMaterialParameterInfo(TEXT("Emissive")), BakeOutput.EmissiveTexture);
        }

        BakedMIC->PostEditChange();
        FAssetRegistryModule::AssetCreated(BakedMIC);

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(MICPackage, BakedMIC,
            *FPackageName::LongPackageNameToFilename(MICPackageName, FPackageName::GetAssetPackageExtension()),
            SaveArgs);

        MeshComp->SetMaterial(0, BakedMIC);

        // Record in manifest
        FDecalBakeResult Result;
        Result.MeshPath = StaticMesh->GetPathName();
        Result.OriginalMaterialPath = OriginalMaterialPath;
        Result.BakedMaterialPath = BakedMIC->GetPathName();
        Result.UVChannel = UVStatus.BakeUVChannel;
        Result.Resolution = Settings->OutputResolution;
        Result.BakeTime = FDateTime::Now();
        for (const FDecalMeshPair& Pair : Pairs)
        {
            if (UDecalComponent* Decal = Pair.DecalComponent.Get())
            {
                Result.DecalActorNames.Add(Decal->GetOwner()->GetName());
            }
        }
        Manifest.Entries.Add(Result);
    }

    // Save manifest to disk
    FString ManifestJson;
    FJsonObjectConverter::UStructToJsonObjectString(Manifest, ManifestJson);
    FString ManifestPath = FPaths::ProjectContentDir() / Settings->OutputPath / TEXT("DecalBakeManifest.json");
    FFileHelper::SaveStringToFile(ManifestJson, *ManifestPath);

    UE_LOG(LogTemp, Log, TEXT("DecalBaker: Bake complete - %d meshes processed"), Manifest.Entries.Num());
    return Manifest;
}

void UDecalBakerSubsystem::RevertBake(const FDecalBakeManifest& Manifest)
{
    for (const FDecalBakeResult& Entry : Manifest.Entries)
    {
        UMaterialInterface* OriginalMat = LoadObject<UMaterialInterface>(
            nullptr, *Entry.OriginalMaterialPath);
        if (!OriginalMat)
        {
            UE_LOG(LogTemp, Warning, TEXT("DecalBaker: Cannot find original material %s"),
                *Entry.OriginalMaterialPath);
            continue;
        }

        UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *Entry.MeshPath);
        if (!StaticMesh) continue;

        for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
        {
            if (It->GetStaticMesh() == StaticMesh)
            {
                UMaterialInterface* CurrentMat = It->GetMaterial(0);
                if (CurrentMat && CurrentMat->GetPathName() == Entry.BakedMaterialPath)
                {
                    It->SetMaterial(0, OriginalMat);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("DecalBaker: Reverted %d baked meshes"), Manifest.Entries.Num());
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
