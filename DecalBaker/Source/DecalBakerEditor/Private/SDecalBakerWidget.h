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

private:
    FReply OnBakeSelectedClicked();
    FReply OnBakeAllClicked();
    FReply OnRevertClicked();

    void ExecuteBake(const TArray<UStaticMeshComponent*>& Scope);

    FDecalBakeManifest LastManifest;
};
