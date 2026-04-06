#include "TextureBaker.h"
#include "DecalBakerLog.h"
#include "DecalBakerSettings.h"
#include "DecalProjection.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/Canvas.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "StaticMeshResources.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"
#include "MeshPassProcessor.h"

// Shader declarations for DecalBakeVS and DecalBakePS
class FDecalBakeVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FDecalBakeVS);
    SHADER_USE_PARAMETER_STRUCT(FDecalBakeVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(float, UVChannelIndex)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FDecalBakePS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FDecalBakePS);
    SHADER_USE_PARAMETER_STRUCT(FDecalBakePS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(FMatrix44f, DecalProjectionMatrix)
        SHADER_PARAMETER(FVector3f, DecalForwardDir)
        SHADER_PARAMETER(float, DecalOpacity)
        SHADER_PARAMETER(float, HasBaseColor)
        SHADER_PARAMETER(float, HasNormal)
        SHADER_PARAMETER(float, HasRoughness)
        SHADER_PARAMETER(float, HasMetallic)
        SHADER_PARAMETER(float, HasEmissive)
        SHADER_PARAMETER_TEXTURE(Texture2D, DecalBaseColorTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, DecalBaseColorSampler)
        SHADER_PARAMETER_TEXTURE(Texture2D, DecalNormalTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, DecalNormalSampler)
        SHADER_PARAMETER_TEXTURE(Texture2D, DecalRoughnessTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, DecalRoughnessSampler)
        SHADER_PARAMETER_TEXTURE(Texture2D, DecalMetallicTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, DecalMetallicSampler)
        SHADER_PARAMETER_TEXTURE(Texture2D, DecalEmissiveTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, DecalEmissiveSampler)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

IMPLEMENT_GLOBAL_SHADER(FDecalBakeVS, "/DecalBaker/DecalBakeVS.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDecalBakePS, "/DecalBaker/DecalBakePS.usf", "MainPS", SF_Pixel);

FTextureBaker::FBakeOutput FTextureBaker::BakeMesh(const FBakeInput& Input)
{
    FBakeOutput Output;
    if (!Input.MeshComponent || !Input.MeshComponent->GetStaticMesh())
    {
        UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: Invalid mesh component in BakeMesh"));
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

    if (Input.DecalPairs.Num() == 0)
    {
        UE_LOG(LogDecalBaker, Warning, TEXT("DecalBaker: No decal pairs for mesh %s, skipping"), *StaticMesh->GetName());
        return Output;
    }

    // Check which channels any decal actually provides
    bool bAnyBaseColor = false, bAnyNormal = false, bAnyRoughness = false;
    bool bAnyMetallic = false, bAnyEmissive = false;
    for (const FDecalMeshPair& Pair : Input.DecalPairs)
    {
        UDecalComponent* Decal = Pair.DecalComponent.Get();
        if (!Decal) continue;
        UMaterialInterface* DecalMat = Decal->GetDecalMaterial();
        if (!DecalMat) continue;
        bAnyBaseColor  = bAnyBaseColor  || (GetTextureFromMaterial(DecalMat, MP_BaseColor) != nullptr);
        bAnyNormal     = bAnyNormal     || (GetTextureFromMaterial(DecalMat, MP_Normal) != nullptr);
        bAnyRoughness  = bAnyRoughness  || (GetTextureFromMaterial(DecalMat, MP_Roughness) != nullptr);
        bAnyMetallic   = bAnyMetallic   || (GetTextureFromMaterial(DecalMat, MP_Metallic) != nullptr);
        bAnyEmissive   = bAnyEmissive   || (GetTextureFromMaterial(DecalMat, MP_EmissiveColor) != nullptr);
    }

    UObject* Outer = GetTransientPackage();

    // Only allocate render targets for channels that are enabled AND that decals provide
    UTextureRenderTarget2D* RT_BaseColor = nullptr;
    UTextureRenderTarget2D* RT_Normal = nullptr;
    UTextureRenderTarget2D* RT_Roughness = nullptr;
    UTextureRenderTarget2D* RT_Metallic = nullptr;
    UTextureRenderTarget2D* RT_Emissive = nullptr;
    UTextureRenderTarget2D* RT_Opacity = nullptr;

    if (Settings->bBakeBaseColor && bAnyBaseColor)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_BaseColor);
        RT_BaseColor = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeNormal && bAnyNormal)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_Normal);
        RT_Normal = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeRoughness && bAnyRoughness)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_Roughness);
        RT_Roughness = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeMetallic && bAnyMetallic)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_Metallic);
        RT_Metallic = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeEmissive && bAnyEmissive)
    {
        UTexture2D* BaseTex = GetTextureFromMaterial(BaseMaterial, MP_EmissiveColor);
        RT_Emissive = CreateInitializedRT(Outer, Res, BaseTex);
    }
    if (Settings->bBakeOpacity)
    {
        RT_Opacity = CreateInitializedRT(Outer, Res, nullptr);
    }

    // Verify we have render data
    if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
    {
        UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: No render data for mesh %s"), *StaticMesh->GetName());
        return Output;
    }

    const int32 BakeUVChannel = Input.BakeUVChannel;

    // GPU bake: for each decal, draw mesh in UV space with decal projection
    for (const FDecalMeshPair& Pair : Input.DecalPairs)
    {
        UDecalComponent* Decal = Pair.DecalComponent.Get();
        if (!Decal) continue;

        UMaterialInterface* DecalMaterial = Decal->GetDecalMaterial();
        if (!DecalMaterial) continue;

        FMatrix ProjMatrix = Pair.ProjectionMatrix;
        FVector ForwardDir = FDecalProjection::GetDecalForwardDir(Decal->GetComponentTransform());

        // Get decal textures per channel
        UTexture2D* DecalBaseColorTex = GetTextureFromMaterial(DecalMaterial, MP_BaseColor);
        UTexture2D* DecalNormalTex = GetTextureFromMaterial(DecalMaterial, MP_Normal);
        UTexture2D* DecalRoughnessTex = GetTextureFromMaterial(DecalMaterial, MP_Roughness);
        UTexture2D* DecalMetallicTex = GetTextureFromMaterial(DecalMaterial, MP_Metallic);
        UTexture2D* DecalEmissiveTex = GetTextureFromMaterial(DecalMaterial, MP_EmissiveColor);

        float HasBaseColorF = DecalBaseColorTex ? 1.0f : 0.0f;
        float HasNormalF = DecalNormalTex ? 1.0f : 0.0f;
        float HasRoughnessF = DecalRoughnessTex ? 1.0f : 0.0f;
        float HasMetallicF = DecalMetallicTex ? 1.0f : 0.0f;
        float HasEmissiveF = DecalEmissiveTex ? 1.0f : 0.0f;

        // Enqueue render commands to draw mesh using DecalBakeVS/PS
        ENQUEUE_RENDER_COMMAND(DecalBakeCommand)(
            [ProjMatrix, ForwardDir, BakeUVChannel,
             RT_BaseColor, RT_Normal, RT_Roughness, RT_Metallic, RT_Emissive, RT_Opacity,
             DecalBaseColorTex, DecalNormalTex, DecalRoughnessTex, DecalMetallicTex, DecalEmissiveTex,
             HasBaseColorF, HasNormalF, HasRoughnessF, HasMetallicF, HasEmissiveF,
             StaticMesh](FRHICommandListImmediate& RHICmdList)
            {
                FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
                TShaderMapRef<FDecalBakeVS> VertexShader(ShaderMap);
                TShaderMapRef<FDecalBakePS> PixelShader(ShaderMap);

                if (!VertexShader.IsValid() || !PixelShader.IsValid())
                {
                    return;
                }

                // Set up MRT render targets
                FRHIRenderPassInfo RPInfo;
                int32 RTIndex = 0;
                auto AddRT = [&](UTextureRenderTarget2D* RT)
                {
                    if (RT && RT->GetRenderTargetResource())
                    {
                        FRHITexture* RHITex = RT->GetRenderTargetResource()->GetRenderTargetTexture();
                        if (RHITex)
                        {
                            RPInfo.ColorRenderTargets[RTIndex].RenderTarget = RHITex;
                            RPInfo.ColorRenderTargets[RTIndex].Action = ERenderTargetActions::Load_Store;
                        }
                    }
                    RTIndex++;
                };

                AddRT(RT_BaseColor);
                AddRT(RT_Normal);
                AddRT(RT_Roughness);  // Roughness+Metallic packed into RT2
                AddRT(RT_Emissive);
                AddRT(RT_Opacity);

                RHICmdList.BeginRenderPass(RPInfo, TEXT("DecalBake"));

                // Set blend state for SrcAlpha/InvSrcAlpha compositing
                FGraphicsPipelineStateInitializer GraphicsPSOInit;
                RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
                GraphicsPSOInit.BlendState = TStaticBlendState<
                    CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha
                >::GetRHI();
                GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
                GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                GraphicsPSOInit.PrimitiveType = PT_TriangleList;

                GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
                GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

                SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

                // Set vertex shader parameters
                FDecalBakeVS::FParameters VSParams;
                VSParams.UVChannelIndex = (float)BakeUVChannel;
                SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VSParams);

                // Set pixel shader parameters
                FDecalBakePS::FParameters PSParams;
                PSParams.DecalProjectionMatrix = FMatrix44f(ProjMatrix);
                PSParams.DecalForwardDir = FVector3f(ForwardDir);
                PSParams.DecalOpacity = 1.0f;
                PSParams.HasBaseColor = HasBaseColorF;
                PSParams.HasNormal = HasNormalF;
                PSParams.HasRoughness = HasRoughnessF;
                PSParams.HasMetallic = HasMetallicF;
                PSParams.HasEmissive = HasEmissiveF;

                // Bind decal textures (use black fallback if null)
                FRHITexture* BlackTex = GBlackTexture->TextureRHI;
                FRHISamplerState* LinearSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();

                auto GetTexRHI = [BlackTex](UTexture2D* Tex) -> FRHITexture*
                {
                    if (Tex && Tex->GetResource() && Tex->GetResource()->TextureRHI)
                    {
                        return Tex->GetResource()->TextureRHI;
                    }
                    return BlackTex;
                };

                PSParams.DecalBaseColorTex = GetTexRHI(DecalBaseColorTex);
                PSParams.DecalBaseColorSampler = LinearSampler;
                PSParams.DecalNormalTex = GetTexRHI(DecalNormalTex);
                PSParams.DecalNormalSampler = LinearSampler;
                PSParams.DecalRoughnessTex = GetTexRHI(DecalRoughnessTex);
                PSParams.DecalRoughnessSampler = LinearSampler;
                PSParams.DecalMetallicTex = GetTexRHI(DecalMetallicTex);
                PSParams.DecalMetallicSampler = LinearSampler;
                PSParams.DecalEmissiveTex = GetTexRHI(DecalEmissiveTex);
                PSParams.DecalEmissiveSampler = LinearSampler;

                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PSParams);

                // Draw the mesh geometry using its vertex/index buffers
                const FStaticMeshLODResources& LOD = StaticMesh->GetRenderData()->LODResources[0];
                const FPositionVertexBuffer& PositionVB = LOD.VertexBuffers.PositionVertexBuffer;
                const FRawStaticIndexBuffer& IndexBuffer = LOD.IndexBuffer;

                RHICmdList.SetStreamSource(0, PositionVB.VertexBufferRHI, 0);

                RHICmdList.DrawIndexedPrimitive(
                    IndexBuffer.IndexBufferRHI,
                    0,
                    0,
                    PositionVB.GetNumVertices(),
                    0,
                    LOD.GetNumTriangles(),
                    1
                );

                RHICmdList.EndRenderPass();
            }
        );
    }

    FlushRenderingCommands();

    // Convert render targets to static textures and release RTs
    FString MeshName = StaticMesh->GetName();

    if (RT_BaseColor)
    {
        Output.BaseColorTexture = RenderTargetToTexture(RT_BaseColor,
            Input.OutputPath, MeshName + TEXT("_BaseColor"));
        RT_BaseColor->ReleaseResource();
        RT_BaseColor->MarkAsGarbage();
    }
    if (RT_Normal)
    {
        Output.NormalTexture = RenderTargetToTexture(RT_Normal,
            Input.OutputPath, MeshName + TEXT("_Normal"));
        RT_Normal->ReleaseResource();
        RT_Normal->MarkAsGarbage();
    }
    if (RT_Roughness)
    {
        Output.RoughnessTexture = RenderTargetToTexture(RT_Roughness,
            Input.OutputPath, MeshName + TEXT("_Roughness"));
        RT_Roughness->ReleaseResource();
        RT_Roughness->MarkAsGarbage();
    }
    if (RT_Metallic)
    {
        Output.MetallicTexture = RenderTargetToTexture(RT_Metallic,
            Input.OutputPath, MeshName + TEXT("_Metallic"));
        RT_Metallic->ReleaseResource();
        RT_Metallic->MarkAsGarbage();
    }
    if (RT_Emissive)
    {
        Output.EmissiveTexture = RenderTargetToTexture(RT_Emissive,
            Input.OutputPath, MeshName + TEXT("_Emissive"));
        RT_Emissive->ReleaseResource();
        RT_Emissive->MarkAsGarbage();
    }
    if (RT_Opacity)
    {
        Output.OpacityTexture = RenderTargetToTexture(RT_Opacity,
            Input.OutputPath, MeshName + TEXT("_Opacity"));
        RT_Opacity->ReleaseResource();
        RT_Opacity->MarkAsGarbage();
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

    // Draw the base texture onto the RT so the final result is original + decal composite
    if (BaseTexture)
    {
        UKismetRenderingLibrary::DrawMaterialToRenderTarget(
            GEngine->GetCurrentPlayWorld() ? GEngine->GetCurrentPlayWorld() : GEditor->GetEditorWorldContext().World(),
            RT,
            UMaterialInterface::GetDefaultMaterial(MD_Surface)
        );
        // Use a canvas draw to blit the base texture onto the RT
        FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();
        if (RTResource)
        {
            ENQUEUE_RENDER_COMMAND(DrawBaseTexture)(
                [RTResource, BaseTexture, Resolution](FRHICommandListImmediate& RHICmdList)
                {
                    FRHITexture* DestTex = RTResource->GetRenderTargetTexture();
                    if (!DestTex || !BaseTexture->GetResource() || !BaseTexture->GetResource()->TextureRHI)
                    {
                        return;
                    }

                    FRHICopyTextureInfo CopyInfo;
                    CopyInfo.Size = FIntVector(
                        FMath::Min(Resolution, (int32)BaseTexture->GetSizeX()),
                        FMath::Min(Resolution, (int32)BaseTexture->GetSizeY()),
                        1
                    );
                    RHICmdList.CopyTexture(
                        BaseTexture->GetResource()->TextureRHI,
                        DestTex,
                        CopyInfo
                    );
                }
            );
            FlushRenderingCommands();
        }
    }

    return RT;
}

UTexture2D* FTextureBaker::GetTextureFromMaterial(
    UMaterialInterface* Material,
    EMaterialProperty Property)
{
    if (!Material) return nullptr;

    // Use GetTexturesInPropertyChain to get the texture actually connected to the requested property
    TArray<UTexture*> PropertyTextures;
    Material->GetTexturesInPropertyChain(Property, PropertyTextures, nullptr, nullptr);

    for (UTexture* Tex : PropertyTextures)
    {
        if (UTexture2D* Tex2D = Cast<UTexture2D>(Tex))
        {
            return Tex2D;
        }
    }

    return nullptr;
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
