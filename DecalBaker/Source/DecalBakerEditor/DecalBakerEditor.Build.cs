using UnrealBuildTool;

public class DecalBakerEditor : ModuleRules
{
    public DecalBakerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "DecalBakerRuntime",
            "UnrealEd",
            "EditorStyle",
            "ToolMenus",
            "LevelEditor",
            "WorkspaceMenuStructure",
            "MaterialBaking",
            "PropertyEditor"
        });
    }
}
