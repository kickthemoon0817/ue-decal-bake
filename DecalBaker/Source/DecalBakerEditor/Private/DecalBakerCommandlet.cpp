#include "DecalBakerCommandlet.h"
#include "DecalBakerSubsystem.h"
#include "DecalBakerSettings.h"
#include "Engine/World.h"
#include "Editor.h"
#include "FileHelpers.h"

UDecalBakerCommandlet::UDecalBakerCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UDecalBakerCommandlet::Main(const FString& Params)
{
    TArray<FString> Tokens;
    TArray<FString> Switches;
    TMap<FString, FString> ParamMap;
    ParseCommandLine(*Params, Tokens, Switches, ParamMap);

    FString MapPath = ParamMap.FindRef(TEXT("map"));
    FString OutputPath = ParamMap.FindRef(TEXT("output"));
    FString ResolutionStr = ParamMap.FindRef(TEXT("resolution"));
    FString UVStrategyStr = ParamMap.FindRef(TEXT("uvstrategy"));

    if (MapPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("DecalBaker: -map parameter is required"));
        return 1;
    }

    UDecalBakerSettings* Settings = GetMutableDefault<UDecalBakerSettings>();
    if (!OutputPath.IsEmpty())
    {
        Settings->OutputPath = OutputPath;
    }
    if (!ResolutionStr.IsEmpty())
    {
        Settings->OutputResolution = FCString::Atoi(*ResolutionStr);
    }
    if (UVStrategyStr == TEXT("auto"))
    {
        Settings->UVStrategy = EDecalBakerUVStrategy::Auto;
    }
    else if (UVStrategyStr == TEXT("uv0"))
    {
        Settings->UVStrategy = EDecalBakerUVStrategy::ForceUV0;
    }
    else if (UVStrategyStr == TEXT("uv1"))
    {
        Settings->UVStrategy = EDecalBakerUVStrategy::ForceUV1;
    }
    else if (UVStrategyStr == TEXT("generate"))
    {
        Settings->UVStrategy = EDecalBakerUVStrategy::ForceGenerate;
    }

    if (!GEditor->Map_Load(*MapPath))
    {
        UE_LOG(LogTemp, Error, TEXT("DecalBaker: Failed to load map %s"), *MapPath);
        return 1;
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("DecalBaker: No world available"));
        return 1;
    }

    UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();
    TArray<UStaticMeshComponent*> Empty;
    FDecalBakeManifest Manifest = Subsystem->BakeDecals(World, Empty);

    UE_LOG(LogTemp, Log, TEXT("DecalBaker: Commandlet complete - %d meshes baked"), Manifest.Entries.Num());

    FEditorFileUtils::SaveCurrentLevel();

    return 0;
}
