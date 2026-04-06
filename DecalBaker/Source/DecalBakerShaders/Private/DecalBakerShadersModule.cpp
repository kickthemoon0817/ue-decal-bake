#include "CoreMinimal.h"
#include "DecalBakerLog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FDecalBakerShadersModule"

class FDecalBakerShadersModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("DecalBaker"));
        if (!Plugin)
        {
            UE_LOG(LogDecalBaker, Error, TEXT("DecalBaker: Cannot find DecalBaker plugin for shader directory mapping"));
            return;
        }

        FString PluginShaderDir = FPaths::Combine(
            Plugin->GetBaseDir(),
            TEXT("Source/DecalBakerShaders/Shaders")
        );
        AddShaderSourceDirectoryMapping(TEXT("/DecalBaker"), PluginShaderDir);
    }

    virtual void ShutdownModule() override
    {
    }
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDecalBakerShadersModule, DecalBakerShaders)
