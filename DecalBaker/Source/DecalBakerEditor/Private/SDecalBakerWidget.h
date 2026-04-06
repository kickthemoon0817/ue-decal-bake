#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "DecalBakerTypes.h"

class UDecalBakerSettings;

class SDecalBakerWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDecalBakerWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** Get selected static mesh components from the editor selection (shared helper) */
    static TArray<UStaticMeshComponent*> GetSelectedStaticMeshComponents();

private:
    FReply OnBakeSelectedClicked();
    FReply OnBakeAllClicked();
    FReply OnRevertClicked();

    void ExecuteBake(const TArray<UStaticMeshComponent*>& Scope);

    bool HasSelection() const;
    bool HasManifest() const;

    void SetStatusText(const FText& Text);

    FDecalBakeManifest LastManifest;
    TSharedPtr<STextBlock> StatusTextBlock;
};
