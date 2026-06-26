using UnrealBuildTool;

public class VoraxiaPlanet : ModuleRules
{
	public VoraxiaPlanet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"GeometryFramework",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"GeometryCore"
			}
		);
	}
}
