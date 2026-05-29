// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StudyProject : ModuleRules
{
	public StudyProject(ReadOnlyTargetRules Target) : base(Target)
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
			"StudyProject",
			"StudyProject/Variant_Platforming",
			"StudyProject/Variant_Platforming/Animation",
			"StudyProject/Variant_Combat",
			"StudyProject/Variant_Combat/AI",
			"StudyProject/Variant_Combat/Animation",
			"StudyProject/Variant_Combat/Gameplay",
			"StudyProject/Variant_Combat/Interfaces",
			"StudyProject/Variant_Combat/UI",
			"StudyProject/Variant_SideScrolling",
			"StudyProject/Variant_SideScrolling/AI",
			"StudyProject/Variant_SideScrolling/Gameplay",
			"StudyProject/Variant_SideScrolling/Interfaces",
			"StudyProject/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
