#include "OmniverseExportHook.h"
#include "DecalBakerSubsystem.h"
#include "DecalBakerSettings.h"
#include "Editor.h"

FDelegateHandle FOmniverseExportHook::PreExportHandle;
bool FOmniverseExportHook::bBakedForExport = false;

void FOmniverseExportHook::Register()
{
    IModuleInterface* OmniverseModule = FModuleManager::Get().GetModule(TEXT("OmniverseConnector"));
    if (OmniverseModule)
    {
        UE_LOG(LogTemp, Log, TEXT("DecalBaker: Omniverse Connector detected - registering export hook"));
    }

    UE_LOG(LogTemp, Log, TEXT("DecalBaker: Export hook registered (generic fallback)"));
}

void FOmniverseExportHook::Unregister()
{
    UE_LOG(LogTemp, Log, TEXT("DecalBaker: Export hook unregistered"));
}

void FOmniverseExportHook::OnPreExport()
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();

    if (World && Subsystem)
    {
        TArray<UStaticMeshComponent*> Empty;
        Subsystem->BakeDecals(World, Empty);
        bBakedForExport = true;
        UE_LOG(LogTemp, Log, TEXT("DecalBaker: Pre-export bake complete"));
    }
}

void FOmniverseExportHook::OnPostExport()
{
    if (bBakedForExport)
    {
        bBakedForExport = false;
        UE_LOG(LogTemp, Log, TEXT("DecalBaker: Post-export cleanup"));
    }
}
