#include "DecalBakerCommandlet.h"
#include "DecalBakerLog.h"
#include "DecalBakerSubsystem.h"
#include "DecalBakerSettings.h"
#include "Engine/World.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Misc/Paths.h"

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
        UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: -map parameter is required"));
        return 1;
    }

    // Copy settings instead of mutating the CDO
    UDecalBakerSettings* Settings = GetMutableDefault<UDecalBakerSettings>();
    UDecalBakerSettings SettingsCopy = *Settings;

    if (!OutputPath.IsEmpty())
    {
        // Validate output path — reject traversal attempts
        FPaths::NormalizeDirectoryName(OutputPath);
        if (OutputPath.Contains(TEXT("..")))
        {
            UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: Invalid output path '%s' — path traversal not allowed"), *OutputPath);
            return 1;
        }
        SettingsCopy.OutputPath = OutputPath;
    }

    if (!ResolutionStr.IsEmpty())
    {
        int32 ParsedRes = FCString::Atoi(*ResolutionStr);
        SettingsCopy.OutputResolution = FMath::Clamp(ParsedRes, 256, 8192);
        if (ParsedRes != SettingsCopy.OutputResolution)
        {
            UE_LOG(LogDecalBaker, Warning, TEXT("DecalBaker: Resolution clamped from %d to %d"),
                ParsedRes, SettingsCopy.OutputResolution);
        }
    }

    if (UVStrategyStr == TEXT("auto"))
    {
        SettingsCopy.UVStrategy = EDecalBakerUVStrategy::Auto;
    }
    else if (UVStrategyStr == TEXT("uv0"))
    {
        SettingsCopy.UVStrategy = EDecalBakerUVStrategy::ForceUV0;
    }
    else if (UVStrategyStr == TEXT("uv1"))
    {
        SettingsCopy.UVStrategy = EDecalBakerUVStrategy::ForceUV1;
    }
    else if (UVStrategyStr == TEXT("generate"))
    {
        SettingsCopy.UVStrategy = EDecalBakerUVStrategy::ForceGenerate;
    }

    // Apply the copy back temporarily for the bake (subsystem reads GetDefault)
    *Settings = SettingsCopy;

    FEditorFileUtils::LoadMap(MapPath, false, true);

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: No world available"));
        return 1;
    }

    UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();
    TArray<UStaticMeshComponent*> Empty;
    FDecalBakeManifest Manifest = Subsystem->BakeDecals(World, Empty);

    UE_LOG(LogDecalBaker, Log, TEXT("DecalBaker: Commandlet complete - %d meshes baked"), Manifest.Entries.Num());

    FEditorFileUtils::SaveCurrentLevel();

    return 0;
}
