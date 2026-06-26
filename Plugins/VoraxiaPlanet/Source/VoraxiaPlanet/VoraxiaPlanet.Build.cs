using UnrealBuildTool;

public class VoraxiaPlanet : ModuleRules
{
    public VoraxiaPlanet(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Keep the first compile boundary deliberately small. Add dependencies only
        // when a concrete feature needs them, rather than front-loading the plugin.
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core"
        });
    }
}
