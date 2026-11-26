using UnrealBuildTool;
public class EidolonTarget : TargetRules
{
    public EidolonTarget(TargetInfo Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        Type = TargetType.Game;
        ExtraModuleNames.Add("Eidolon");
    }
}
