#include "OmniverseExportHook.h"
#include "DecalBakerLog.h"
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
        UE_LOG(LogDecalBaker, Log, TEXT("DecalBaker: Omniverse Connector detected"));
        // TODO: Bind to Omniverse Connector's pre-export delegate when the API is available.
        // Example: PreExportHandle = IOmniverseConnector::Get().OnPreExport().AddStatic(&FOmniverseExportHook::OnPreExport);
    }
    else
    {
        UE_LOG(LogDecalBaker, Log, TEXT("DecalBaker: Omniverse Connector not found — export hook inactive"));
    }
}

void FOmniverseExportHook::Unregister()
{
    if (PreExportHandle.IsValid())
    {
        // TODO: Unbind from delegate when implemented
        PreExportHandle.Reset();
    }
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
        UE_LOG(LogDecalBaker, Log, TEXT("DecalBaker: Pre-export bake complete"));
    }
}

void FOmniverseExportHook::OnPostExport()
{
    if (bBakedForExport)
    {
        bBakedForExport = false;
        UE_LOG(LogDecalBaker, Log, TEXT("DecalBaker: Post-export cleanup"));
    }
}
