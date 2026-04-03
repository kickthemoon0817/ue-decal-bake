#include "CoreMinimal.h"
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
        FString PluginShaderDir = FPaths::Combine(
            IPluginManager::Get().FindPlugin(TEXT("DecalBaker"))->GetBaseDir(),
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
