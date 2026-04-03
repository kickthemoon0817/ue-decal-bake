#include "SDecalBakerWidget.h"
#include "DecalBakerSubsystem.h"
#include "DecalBakerSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Editor.h"
#include "Selection.h"
#include "Engine/StaticMeshActor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Components/StaticMeshComponent.h"

#define LOCTEXT_NAMESPACE "SDecalBakerWidget"

void SDecalBakerWidget::Construct(const FArguments& InArgs)
{
    UDecalBakerSettings* Settings = GetMutableDefault<UDecalBakerSettings>();

    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bAllowSearch = false;
    DetailsArgs.bShowOptions = false;
    DetailsArgs.bHideSelectionTip = true;
    TSharedRef<IDetailsView> SettingsView = PropertyModule.CreateDetailView(DetailsArgs);
    SettingsView->SetObject(Settings);

    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        .Padding(8)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 8)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("Header", "Decal Baker"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 8)
            [
                SettingsView
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 4)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .Padding(0, 0, 4, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("BakeSelected", "Bake Selected"))
                    .OnClicked(this, &SDecalBakerWidget::OnBakeSelectedClicked)
                ]
                + SHorizontalBox::Slot()
                .Padding(4, 0, 0, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("BakeAll", "Bake All"))
                    .OnClicked(this, &SDecalBakerWidget::OnBakeAllClicked)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 4)
            [
                SNew(SButton)
                .Text(LOCTEXT("Revert", "Revert All Baked Materials"))
                .OnClicked(this, &SDecalBakerWidget::OnRevertClicked)
            ]
        ]
    ];
}

FReply SDecalBakerWidget::OnBakeSelectedClicked()
{
    TArray<UStaticMeshComponent*> SelectedMeshes;

    USelection* Selection = GEditor->GetSelectedActors();
    for (int32 i = 0; i < Selection->Num(); ++i)
    {
        AActor* Actor = Cast<AActor>(Selection->GetSelectedObject(i));
        if (Actor)
        {
            TArray<UStaticMeshComponent*> Components;
            Actor->GetComponents<UStaticMeshComponent>(Components);
            SelectedMeshes.Append(Components);
        }
    }

    ExecuteBake(SelectedMeshes);
    return FReply::Handled();
}

FReply SDecalBakerWidget::OnBakeAllClicked()
{
    TArray<UStaticMeshComponent*> Empty;
    ExecuteBake(Empty);
    return FReply::Handled();
}

FReply SDecalBakerWidget::OnRevertClicked()
{
    UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();
    if (Subsystem)
    {
        Subsystem->RevertBake(LastManifest);
    }
    return FReply::Handled();
}

void SDecalBakerWidget::ExecuteBake(const TArray<UStaticMeshComponent*>& Scope)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();

    if (World && Subsystem)
    {
        LastManifest = Subsystem->BakeDecals(World, Scope);
        UE_LOG(LogTemp, Log, TEXT("DecalBaker: Baked %d meshes"), LastManifest.Entries.Num());
    }
}

#undef LOCTEXT_NAMESPACE
