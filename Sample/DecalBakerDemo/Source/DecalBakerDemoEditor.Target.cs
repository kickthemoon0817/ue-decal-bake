using UnrealBuildTool;

public class DecalBakerDemoEditorTarget : TargetRules
{
    public DecalBakerDemoEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("DecalBakerDemo");
    }
}
