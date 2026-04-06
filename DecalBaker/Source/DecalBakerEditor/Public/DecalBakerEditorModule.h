#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FDecalBakerEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedRef<SDockTab> OnSpawnTab(const FSpawnTabArgs& Args);
    void RegisterMenuExtensions();
    void OnToolbarButtonClicked();

    TSharedPtr<FUICommandList> CommandList;

    static const FName DecalBakerTabName;
};
