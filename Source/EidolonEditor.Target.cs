using UnrealBuildTool;
public class EidolonEditorTarget : TargetRules
{
    public EidolonEditorTarget(TargetInfo Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        Type = TargetType.Editor;
        ExtraModuleNames.Add("Eidolon");
    }
}
