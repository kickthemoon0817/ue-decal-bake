#include "SDecalBakerWidget.h"
#include "DecalBakerLog.h"
#include "DecalBakerSubsystem.h"
#include "DecalBakerSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Editor.h"
#include "Selection.h"
#include "Engine/StaticMeshActor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Components/StaticMeshComponent.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SDecalBakerWidget"

void SDecalBakerWidget::Construct(const FArguments& InArgs)
{
    UDecalBakerSettings* Settings = GetMutableDefault<UDecalBakerSettings>();

    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
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
                .Font(FAppStyle::GetFontStyle("NormalFontBold"))
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
                    .IsEnabled(this, &SDecalBakerWidget::HasSelection)
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
                .Text(LOCTEXT("Revert", "Undo Last Bake"))
                .IsEnabled(this, &SDecalBakerWidget::HasManifest)
                .OnClicked(this, &SDecalBakerWidget::OnRevertClicked)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 8, 0, 0)
            [
                SAssignNew(StatusTextBlock, STextBlock)
                .Text(LOCTEXT("StatusReady", "Ready"))
            ]
        ]
    ];
}

TArray<UStaticMeshComponent*> SDecalBakerWidget::GetSelectedStaticMeshComponents()
{
    TArray<UStaticMeshComponent*> SelectedMeshes;
    TSet<UStaticMeshComponent*> Seen;

    USelection* Selection = GEditor->GetSelectedActors();
    for (int32 i = 0; i < Selection->Num(); ++i)
    {
        AActor* Actor = Cast<AActor>(Selection->GetSelectedObject(i));
        if (Actor)
        {
            TArray<UStaticMeshComponent*> Components;
            Actor->GetComponents<UStaticMeshComponent>(Components);
            for (UStaticMeshComponent* Comp : Components)
            {
                bool bAlreadyInSet = false;
                Seen.Add(Comp, &bAlreadyInSet);
                if (!bAlreadyInSet)
                {
                    SelectedMeshes.Add(Comp);
                }
            }
        }
    }
    return SelectedMeshes;
}

FReply SDecalBakerWidget::OnBakeSelectedClicked()
{
    TArray<UStaticMeshComponent*> SelectedMeshes = GetSelectedStaticMeshComponents();

    if (SelectedMeshes.Num() == 0)
    {
        SetStatusText(LOCTEXT("StatusNoSelection", "No meshes selected"));
        return FReply::Handled();
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
    if (LastManifest.Entries.Num() == 0)
    {
        SetStatusText(LOCTEXT("StatusNothingToRevert", "Nothing to revert"));
        return FReply::Handled();
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();
    if (Subsystem && World)
    {
        Subsystem->RevertBake(LastManifest, World);
        SetStatusText(FText::Format(
            LOCTEXT("StatusReverted", "Reverted {0} entries"),
            FText::AsNumber(LastManifest.Entries.Num())));
        LastManifest = FDecalBakeManifest();
    }
    return FReply::Handled();
}

void SDecalBakerWidget::ExecuteBake(const TArray<UStaticMeshComponent*>& Scope)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();

    if (World && Subsystem)
    {
        SetStatusText(LOCTEXT("StatusBaking", "Baking..."));

        LastManifest = Subsystem->BakeDecals(World, Scope);

        if (LastManifest.Entries.Num() > 0)
        {
            SetStatusText(FText::Format(
                LOCTEXT("StatusBakeComplete", "Baked {0} meshes successfully"),
                FText::AsNumber(LastManifest.Entries.Num())));

            FNotificationInfo Info(FText::Format(
                LOCTEXT("NotifBakeComplete", "DecalBaker: Baked {0} meshes"),
                FText::AsNumber(LastManifest.Entries.Num())));
            Info.ExpireDuration = 3.0f;
            FSlateNotificationManager::Get().AddNotification(Info);
        }
        else
        {
            SetStatusText(LOCTEXT("StatusNoPairs", "No decal-mesh pairs found"));
        }
    }
}

bool SDecalBakerWidget::HasSelection() const
{
    return GEditor && GEditor->GetSelectedActors() && GEditor->GetSelectedActors()->Num() > 0;
}

bool SDecalBakerWidget::HasManifest() const
{
    return LastManifest.Entries.Num() > 0;
}

void SDecalBakerWidget::SetStatusText(const FText& Text)
{
    if (StatusTextBlock.IsValid())
    {
        StatusTextBlock->SetText(Text);
    }
}

#undef LOCTEXT_NAMESPACE
