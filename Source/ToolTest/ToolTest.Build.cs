// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ToolTest : ModuleRules
{
	public ToolTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ToolTest",
			"ToolTest/Variant_Platforming",
			"ToolTest/Variant_Platforming/Animation",
			"ToolTest/Variant_Combat",
			"ToolTest/Variant_Combat/AI",
			"ToolTest/Variant_Combat/Animation",
			"ToolTest/Variant_Combat/Gameplay",
			"ToolTest/Variant_Combat/Interfaces",
			"ToolTest/Variant_Combat/UI",
			"ToolTest/Variant_SideScrolling",
			"ToolTest/Variant_SideScrolling/AI",
			"ToolTest/Variant_SideScrolling/Gameplay",
			"ToolTest/Variant_SideScrolling/Interfaces",
			"ToolTest/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
