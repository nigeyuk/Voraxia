using UnrealBuildTool;
using System.Collections.Generic;

public class VoraxiaEditorTarget : TargetRules
{
    public VoraxiaEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        ExtraModuleNames.Add("Voraxia");
    }
}
