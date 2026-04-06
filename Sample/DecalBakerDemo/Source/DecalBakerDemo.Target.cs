using UnrealBuildTool;

public class DecalBakerDemoTarget : TargetRules
{
    public DecalBakerDemoTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("DecalBakerDemo");
    }
}
