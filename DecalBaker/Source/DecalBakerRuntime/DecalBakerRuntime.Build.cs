using UnrealBuildTool;

public class DecalBakerRuntime : ModuleRules
{
    public DecalBakerRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "RenderCore",
            "RHI",
            "MeshDescription",
            "StaticMeshDescription"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "MaterialBaking",
            "Json",
            "JsonUtilities"
        });
    }
}
