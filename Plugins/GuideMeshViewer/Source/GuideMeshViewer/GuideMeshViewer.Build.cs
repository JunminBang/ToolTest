using UnrealBuildTool;

public class GuideMeshViewer : ModuleRules
{
	public GuideMeshViewer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"Persona",
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"WorkspaceMenuStructure",
		});
	}
}
