#include "DecalBakerEditorModule.h"
#include "DecalBakerEditorCommands.h"
#include "SDecalBakerWidget.h"
#include "OmniverseExportHook.h"
#include "DecalBakerLog.h"
#include "DecalBakerSubsystem.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Selection.h"
#include "Components/StaticMeshComponent.h"

#define LOCTEXT_NAMESPACE "FDecalBakerEditorModule"

const FName FDecalBakerEditorModule::DecalBakerTabName(TEXT("DecalBakerTab"));

void FDecalBakerEditorModule::StartupModule()
{
    FDecalBakerEditorCommands::Register();

    CommandList = MakeShareable(new FUICommandList);

    // Wire all commands to actions
    CommandList->MapAction(
        FDecalBakerEditorCommands::Get().OpenPanel,
        FExecuteAction::CreateRaw(this, &FDecalBakerEditorModule::OnToolbarButtonClicked)
    );

    CommandList->MapAction(
        FDecalBakerEditorCommands::Get().BakeSelected,
        FExecuteAction::CreateLambda([]()
        {
            UWorld* World = GEditor->GetEditorWorldContext().World();
            UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();
            if (!World || !Subsystem) return;
            TArray<UStaticMeshComponent*> Selected = SDecalBakerWidget::GetSelectedStaticMeshComponents();
            if (Selected.Num() > 0)
            {
                Subsystem->BakeDecals(World, Selected);
            }
        })
    );

    CommandList->MapAction(
        FDecalBakerEditorCommands::Get().BakeAll,
        FExecuteAction::CreateLambda([]()
        {
            UWorld* World = GEditor->GetEditorWorldContext().World();
            UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();
            if (!World || !Subsystem) return;
            TArray<UStaticMeshComponent*> Empty;
            Subsystem->BakeDecals(World, Empty);
        })
    );

    CommandList->MapAction(
        FDecalBakerEditorCommands::Get().RevertAll,
        FExecuteAction::CreateLambda([]()
        {
            // RevertAll via command requires manifest from subsystem - no-op without active widget
            UE_LOG(LogDecalBaker, Log, TEXT("DecalBaker: Use the panel to revert bakes"));
        })
    );

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        DecalBakerTabName,
        FOnSpawnTab::CreateRaw(this, &FDecalBakerEditorModule::OnSpawnTab))
        .SetDisplayName(LOCTEXT("TabTitle", "Decal Baker"))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
        .SetMenuType(ETabSpawnerMenuType::Enabled);

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
    {
        RegisterMenuExtensions();
    }));

    FOmniverseExportHook::Register();
}

void FDecalBakerEditorModule::ShutdownModule()
{
    FOmniverseExportHook::Unregister();
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DecalBakerTabName);
    FDecalBakerEditorCommands::Unregister();
}

TSharedRef<SDockTab> FDecalBakerEditorModule::OnSpawnTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SDecalBakerWidget)
        ];
}

void FDecalBakerEditorModule::RegisterMenuExtensions()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    // Toolbar button
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(
        "LevelEditor.LevelEditorToolBar.PlayToolBar");

    FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("DecalBaker");
    Section.AddEntry(FToolMenuEntry::InitToolBarButton(
        FDecalBakerEditorCommands::Get().OpenPanel,
        LOCTEXT("ToolbarButton", "Decal Baker"),
        LOCTEXT("ToolbarTooltip", "Open the Decal Baker panel to bake decals into mesh textures"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "MaterialEditor.Apply")
    ));

    // Window menu
    UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window");
    FToolMenuSection& WindowSection = WindowMenu->FindOrAddSection("DecalBaker");
    WindowSection.AddMenuEntryWithCommandList(
        FDecalBakerEditorCommands::Get().OpenPanel,
        CommandList,
        LOCTEXT("WindowMenuItem", "Decal Baker")
    );

    // Right-click context menu — uses shared selection helper
    UToolMenu* ActorContextMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu");
    FToolMenuSection& ContextSection = ActorContextMenu->FindOrAddSection("DecalBaker");
    ContextSection.AddMenuEntry(
        "BakeDecalsToTextures",
        LOCTEXT("ContextBake", "Bake Decals to Textures"),
        LOCTEXT("ContextBakeTooltip", "Bake all decals affecting the selected meshes into their textures"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([]()
        {
            UWorld* World = GEditor->GetEditorWorldContext().World();
            UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();
            if (!World || !Subsystem) return;

            TArray<UStaticMeshComponent*> SelectedMeshes = SDecalBakerWidget::GetSelectedStaticMeshComponents();
            if (SelectedMeshes.Num() > 0)
            {
                Subsystem->BakeDecals(World, SelectedMeshes);
            }
        }))
    );
}

void FDecalBakerEditorModule::OnToolbarButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(DecalBakerTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDecalBakerEditorModule, DecalBakerEditor)
