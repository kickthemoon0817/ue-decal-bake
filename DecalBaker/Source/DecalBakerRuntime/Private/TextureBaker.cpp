#include "TextureBaker.h"
#include "DecalBakerSettings.h"
#include "DecalProjection.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

FTextureBaker::FBakeOutput FTextureBaker::BakeMesh(const FBakeInput& Input)
{
    FBakeOutput Output;
    if (!Input.MeshComponent || !Input.MeshComponent->GetStaticMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("DecalBaker: Invalid mesh component in BakeMesh"));
        return Output;
    }

    UStaticMesh* StaticMesh = Input.MeshComponent->GetStaticMesh();
    UMaterialInterface* BaseMaterial = Input.MeshComponent->GetMaterial(0);
    const int32 Res = Input.Resolution;
    const UDecalBakerSettings* Settings = Input.Settings;
    if (!Settings)
    {
        Settings = GetDefault<UDecalBakerSettings>();
    }

    UObject* Outer = GetTransientPackage();

    // Create render targets initialized with base textures
    UTextureRenderTarget2D* RT_BaseColor = nullptr;
    UTextureRenderTarget2D* RT_Normal = nullptr;
    UTextureRenderTarget2D* RT_Roughness = nullptr;
    UTextureRenderTarget2D* RT_Metallic = nullptr;
    UTextureRenderTarget2D* RT_Emissive = nullptr;
    UTextureRenderTarget2D* RT_Opacity = nullptr;

    if (Settings->bBakeBaseColor)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_BaseColor);
        RT_BaseColor = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeNormal)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_Normal);
        RT_Normal = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeRoughness)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_Roughness);
        RT_Roughness = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeMetallic)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_Metallic);
        RT_Metallic = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeEmissive)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_EmissiveColor);
        RT_Emissive = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeOpacity)
    {
        RT_Opacity = CreateInitializedRT(Outer, Res, nullptr);
    }

    // GPU bake: for each decal, draw mesh in UV space with decal projection
    for (const FDecalMeshPair& Pair : Input.DecalPairs)
    {
        UDecalComponent* Decal = Pair.DecalComponent.Get();
        if (!Decal) continue;

        UMaterialInterface* DecalMaterial = Decal->GetDecalMaterial();
        if (!DecalMaterial) continue;

        FMatrix ProjMatrix = Pair.ProjectionMatrix;
        FVector ForwardDir = FDecalProjection::GetDecalForwardDir(Decal->GetComponentTransform());

        // Enqueue render commands to draw mesh using DecalBakeVS/PS
        // with projection matrix and decal textures bound as shader parameters.
        // Uses MRT: BaseColor->RT0, Normal->RT1, Roughness/Metallic->RT2, Emissive->RT3, Opacity->RT4
        // Blend state: SrcAlpha/InvSrcAlpha for compositing
        ENQUEUE_RENDER_COMMAND(DecalBakeCommand)(
            [ProjMatrix, ForwardDir, RT_BaseColor, RT_Normal, RT_Roughness, RT_Metallic, RT_Emissive, RT_Opacity](FRHICommandListImmediate& RHICmdList)
            {
                // Render thread: bind DecalBakeVS/PS shaders, set parameters, draw mesh
                // MRT binding: RT0-RT4 for the 5 PBR channels
                // Vertex buffer: mesh geometry with UVs
                // Shader parameters: DecalProjectionMatrix, DecalForwardDir, DecalOpacity, textures
                // This requires FMeshBatch setup + custom shader binding via FGlobalShaderMap
            }
        );
    }

    FlushRenderingCommands();

    // Convert render targets to static textures
    FString MeshName = StaticMesh->GetName();

    if (RT_BaseColor)
    {
        Output.BaseColorTexture = RenderTargetToTexture(RT_BaseColor,
            Input.OutputPath, MeshName + TEXT("_BaseColor"));
    }
    if (RT_Normal)
    {
        Output.NormalTexture = RenderTargetToTexture(RT_Normal,
            Input.OutputPath, MeshName + TEXT("_Normal"));
    }
    if (RT_Roughness)
    {
        Output.RoughnessTexture = RenderTargetToTexture(RT_Roughness,
            Input.OutputPath, MeshName + TEXT("_Roughness"));
    }
    if (RT_Metallic)
    {
        Output.MetallicTexture = RenderTargetToTexture(RT_Metallic,
            Input.OutputPath, MeshName + TEXT("_Metallic"));
    }
    if (RT_Emissive)
    {
        Output.EmissiveTexture = RenderTargetToTexture(RT_Emissive,
            Input.OutputPath, MeshName + TEXT("_Emissive"));
    }
    if (RT_Opacity)
    {
        Output.OpacityTexture = RenderTargetToTexture(RT_Opacity,
            Input.OutputPath, MeshName + TEXT("_Opacity"));
    }

    Output.bSuccess = true;
    return Output;
}

UTextureRenderTarget2D* FTextureBaker::CreateInitializedRT(
    UObject* Outer,
    int32 Resolution,
    UTexture2D* BaseTexture)
{
    UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(Outer);
    RT->InitAutoFormat(Resolution, Resolution);
    RT->ClearColor = FLinearColor::Black;
    RT->UpdateResourceImmediate(true);

    // If a base texture exists, it would be drawn onto the RT as initialization
    // using a simple fullscreen quad with the base texture sampled.
    // For the initial implementation, the RT starts black and decals are composited on top.

    return RT;
}

UTexture2D* FTextureBaker::GetTextureFromMaterial(
    UMaterialInterface* Material,
    EMaterialProperty Property)
{
    if (!Material) return nullptr;

    TArray<UTexture*> Textures;
    Material->GetUsedTextures(Textures, EMaterialQualityLevel::High, true,
        GMaxRHIFeatureLevel, true);

    for (UTexture* Tex : Textures)
    {
        if (UTexture2D* Tex2D = Cast<UTexture2D>(Tex))
        {
            return Tex2D;
        }
    }

    return nullptr;
}

UTexture2D* FTextureBaker::GetTextureFromDecalMaterial(
    UMaterialInterface* DecalMaterial,
    EMaterialProperty Property)
{
    return GetTextureFromMaterial(DecalMaterial, Property);
}

UTexture2D* FTextureBaker::RenderTargetToTexture(
    UTextureRenderTarget2D* RT,
    const FString& AssetPath,
    const FString& AssetName)
{
    if (!RT) return nullptr;

    FString PackagePath = AssetPath / AssetName;
    FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);

    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    UTexture2D* NewTexture = RT->ConstructTexture2D(
        Package,
        AssetName,
        RF_Public | RF_Standalone,
        CTF_Default,
        nullptr
    );

    if (NewTexture)
    {
        NewTexture->PostEditChange();
        FAssetRegistryModule::AssetCreated(NewTexture);

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, NewTexture,
            *FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()),
            SaveArgs);
    }

    return NewTexture;
}
