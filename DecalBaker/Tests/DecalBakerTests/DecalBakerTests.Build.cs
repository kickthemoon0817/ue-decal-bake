using UnrealBuildTool;

public class DecalBakerTests : ModuleRules
{
    public DecalBakerTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "AutomationController",
            "DecalBakerRuntime"
        });
    }
}
