#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DecalBakerTypes.h"
#include "DecalBakerSettings.generated.h"

UCLASS(config = DecalBaker, defaultconfig, meta = (DisplayName = "Decal Baker"))
class DECALBAKERRUNTIME_API UDecalBakerSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UDecalBakerSettings();

    /** Output texture resolution (width and height) */
    UPROPERTY(config, EditAnywhere, Category = "Baking",
        meta = (ClampMin = "256", ClampMax = "8192"))
    int32 OutputResolution = 2048;

    /** UV strategy for handling overlapping UVs */
    UPROPERTY(config, EditAnywhere, Category = "Baking")
    EDecalBakerUVStrategy UVStrategy = EDecalBakerUVStrategy::Auto;

    /** Padding in pixels between UV islands to prevent bleeding */
    UPROPERTY(config, EditAnywhere, Category = "Baking",
        meta = (ClampMin = "0", ClampMax = "32"))
    int32 UVPadding = 4;

    /** Output folder path for baked textures */
    UPROPERTY(config, EditAnywhere, Category = "Output")
    FString OutputPath = TEXT("/Game/BakedDecals/");

    /** Bake BaseColor channel */
    UPROPERTY(config, EditAnywhere, Category = "Channels")
    bool bBakeBaseColor = true;

    /** Bake Normal channel */
    UPROPERTY(config, EditAnywhere, Category = "Channels")
    bool bBakeNormal = true;

    /** Bake Roughness channel */
    UPROPERTY(config, EditAnywhere, Category = "Channels")
    bool bBakeRoughness = true;

    /** Bake Metallic channel */
    UPROPERTY(config, EditAnywhere, Category = "Channels")
    bool bBakeMetallic = true;

    /** Bake Emissive channel */
    UPROPERTY(config, EditAnywhere, Category = "Channels")
    bool bBakeEmissive = true;

    /** Bake Opacity channel */
    UPROPERTY(config, EditAnywhere, Category = "Channels")
    bool bBakeOpacity = true;

    virtual FName GetCategoryName() const override { return FName("Plugins"); }
};
