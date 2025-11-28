// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Rouge : ModuleRules
{
	public Rouge(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Rouge",
			"Rouge/Variant_Platforming",
			"Rouge/Variant_Platforming/Animation",
			"Rouge/Variant_Combat",
			"Rouge/Variant_Combat/AI",
			"Rouge/Variant_Combat/Animation",
			"Rouge/Variant_Combat/Gameplay",
			"Rouge/Variant_Combat/Interfaces",
			"Rouge/Variant_Combat/UI",
			"Rouge/Variant_SideScrolling",
			"Rouge/Variant_SideScrolling/AI",
			"Rouge/Variant_SideScrolling/Gameplay",
			"Rouge/Variant_SideScrolling/Interfaces",
			"Rouge/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
