// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Alpakit : ModuleRules
{
	public Alpakit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		PublicDependencyModuleNames.AddRange(new[] {
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new[] {
			"CoreUObject",
			"Engine",
			"Projects",
			"InputCore",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"LevelEditor",
			"PropertyEditor",
			"Settings",
			"UATHelper",
		});
	}
}
