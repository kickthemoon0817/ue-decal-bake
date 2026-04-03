#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

class FDecalBakerEditorCommands : public TCommands<FDecalBakerEditorCommands>
{
public:
    FDecalBakerEditorCommands()
        : TCommands<FDecalBakerEditorCommands>(
            TEXT("DecalBaker"),
            NSLOCTEXT("Contexts", "DecalBaker", "Decal Baker"),
            NAME_None,
            FEditorStyle::GetStyleSetName())
    {
    }

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> OpenPanel;
    TSharedPtr<FUICommandInfo> BakeSelected;
    TSharedPtr<FUICommandInfo> BakeAll;
    TSharedPtr<FUICommandInfo> RevertAll;
};
