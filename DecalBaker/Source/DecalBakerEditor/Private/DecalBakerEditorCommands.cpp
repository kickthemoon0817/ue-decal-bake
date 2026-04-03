#include "DecalBakerEditorCommands.h"

#define LOCTEXT_NAMESPACE "FDecalBakerEditorCommands"

void FDecalBakerEditorCommands::RegisterCommands()
{
    UI_COMMAND(OpenPanel, "Decal Baker", "Open the Decal Baker panel",
        EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(BakeSelected, "Bake Selected", "Bake decals on selected actors",
        EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(BakeAll, "Bake All", "Bake all decals in the level",
        EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(RevertAll, "Revert All", "Revert all baked materials to originals",
        EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
