using UnrealBuildTool;

public class VoraxiaVoxel : ModuleRules
{
    public VoraxiaVoxel(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",                
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "ProceduralMeshComponent",
                "VoraxiaLog",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
            }
        );
    }
}