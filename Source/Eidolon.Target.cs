using UnrealBuildTool;
public class EidolonTarget : TargetRules
{
    public EidolonTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        ExtraModuleNames.Add("Eidolon");
    }
}
