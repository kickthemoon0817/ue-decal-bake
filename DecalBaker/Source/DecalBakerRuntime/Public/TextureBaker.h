#pragma once

#include "CoreMinimal.h"
#include "DecalBakerTypes.h"

class UTextureRenderTarget2D;
class UTexture2D;
class UStaticMeshComponent;
class UMaterialInterface;
class UDecalComponent;
class UDecalBakerSettings;

class DECALBAKERRUNTIME_API FTextureBaker
{
public:
    struct FBakeInput
    {
        UStaticMeshComponent* MeshComponent = nullptr;
        TArray<FDecalMeshPair> DecalPairs;
        int32 BakeUVChannel = 0;
        int32 Resolution = 2048;
        FString OutputPath;
        const UDecalBakerSettings* Settings = nullptr;
    };

    struct FBakeOutput
    {
        UTexture2D* BaseColorTexture = nullptr;
        UTexture2D* NormalTexture = nullptr;
        UTexture2D* RoughnessTexture = nullptr;
        UTexture2D* MetallicTexture = nullptr;
        UTexture2D* EmissiveTexture = nullptr;
        UTexture2D* OpacityTexture = nullptr;
        bool bSuccess = false;
    };

    static FBakeOutput BakeMesh(const FBakeInput& Input);

private:
    static UTextureRenderTarget2D* CreateInitializedRT(
        UObject* Outer,
        int32 Resolution,
        UTexture2D* BaseTexture);

    static UTexture2D* GetTextureFromMaterial(
        UMaterialInterface* Material,
        EMaterialProperty Property);

    static UTexture2D* GetTextureFromDecalMaterial(
        UMaterialInterface* DecalMaterial,
        EMaterialProperty Property);

    static UTexture2D* RenderTargetToTexture(
        UTextureRenderTarget2D* RT,
        const FString& AssetPath,
        const FString& AssetName);
};
