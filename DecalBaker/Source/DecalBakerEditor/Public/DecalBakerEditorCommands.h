#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

class FDecalBakerEditorCommands : public TCommands<FDecalBakerEditorCommands>
{
public:
    FDecalBakerEditorCommands()
        : TCommands<FDecalBakerEditorCommands>(
            TEXT("DecalBaker"),
            NSLOCTEXT("Contexts", "DecalBaker", "Decal Baker"),
            NAME_None,
            FAppStyle::GetAppStyleSetName())
    {
    }

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> OpenPanel;
    TSharedPtr<FUICommandInfo> BakeSelected;
    TSharedPtr<FUICommandInfo> BakeAll;
    TSharedPtr<FUICommandInfo> RevertAll;
};
