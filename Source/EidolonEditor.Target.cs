using UnrealBuildTool;
public class EidolonEditorTarget : TargetRules
{
    public EidolonEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        ExtraModuleNames.Add("Eidolon");
    }
}
