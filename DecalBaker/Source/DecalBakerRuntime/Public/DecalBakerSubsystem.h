#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "DecalBakerTypes.h"
#include "DecalBakerSubsystem.generated.h"

UCLASS()
class DECALBAKERRUNTIME_API UDecalBakerSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "DecalBaker")
    TArray<FDecalMeshPair> DiscoverDecalMeshPairs(
        UWorld* World,
        const TArray<UStaticMeshComponent*>& InScope);

    UFUNCTION(BlueprintCallable, Category = "DecalBaker")
    FDecalBakeManifest BakeDecals(
        UWorld* World,
        const TArray<UStaticMeshComponent*>& InScope);

    UFUNCTION(BlueprintCallable, Category = "DecalBaker")
    void RevertBake(const FDecalBakeManifest& Manifest, UWorld* World);

private:
    TArray<UDecalComponent*> FindAllDecals(UWorld* World) const;

    TArray<UStaticMeshComponent*> FindAffectedMeshes(
        UWorld* World,
        UDecalComponent* Decal,
        const TArray<UStaticMeshComponent*>& InScope) const;
};
