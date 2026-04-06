#pragma once

#include "CoreMinimal.h"
#include "DecalBakerTypes.generated.h"

class UDecalComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EDecalBakerUVStrategy : uint8
{
    Auto        UMETA(DisplayName = "Auto (UV1 -> Generate -> Atlas)"),
    ForceUV0    UMETA(DisplayName = "Force UV0"),
    ForceUV1    UMETA(DisplayName = "Force UV1 (Lightmap)"),
    ForceGenerate UMETA(DisplayName = "Force Generate New UV")
};

UENUM(BlueprintType)
enum class EDecalBakerScope : uint8
{
    Selected    UMETA(DisplayName = "Selected Actors"),
    AllInLevel  UMETA(DisplayName = "All In Level"),
    ByFolder    UMETA(DisplayName = "By Folder")
};

USTRUCT(BlueprintType)
struct DECALBAKERRUNTIME_API FDecalMeshPair
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UDecalComponent> DecalComponent;

    UPROPERTY()
    TWeakObjectPtr<UStaticMeshComponent> MeshComponent;

    /** World-to-decal-local projection matrix (not exposed to Blueprint — FMatrix is not a UPROPERTY type) */
    FMatrix ProjectionMatrix;

    /** Decal sort order for compositing priority */
    UPROPERTY(BlueprintReadOnly, Category = "DecalBaker")
    int32 SortOrder = 0;
};

USTRUCT(BlueprintType)
struct DECALBAKERRUNTIME_API FDecalBakeResult
{
    GENERATED_BODY()

    UPROPERTY()
    FString MeshPath;

    UPROPERTY()
    FString OriginalMaterialPath;

    UPROPERTY()
    FString BakedMaterialPath;

    UPROPERTY()
    int32 UVChannel = 0;

    UPROPERTY()
    int32 Resolution = 2048;

    UPROPERTY()
    TArray<FString> DecalActorNames;

    UPROPERTY()
    FDateTime BakeTime;
};

USTRUCT(BlueprintType)
struct DECALBAKERRUNTIME_API FDecalBakeManifest
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Version = 1;

    UPROPERTY()
    TArray<FDecalBakeResult> Entries;
};

USTRUCT(BlueprintType)
struct DECALBAKERRUNTIME_API FMeshUVStatus
{
    GENERATED_BODY()

    /** True if UV0 has overlapping triangles */
    UPROPERTY(BlueprintReadOnly, Category = "DecalBaker")
    bool bHasOverlap = false;

    /** UV channel index to use for baking (0, 1, or generated) */
    UPROPERTY(BlueprintReadOnly, Category = "DecalBaker")
    int32 BakeUVChannel = 0;

    /** True if a new UV channel was generated */
    UPROPERTY(BlueprintReadOnly, Category = "DecalBaker")
    bool bGeneratedUV = false;
};
