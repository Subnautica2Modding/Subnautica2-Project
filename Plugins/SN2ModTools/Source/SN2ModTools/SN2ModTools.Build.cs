using UnrealBuildTool;

public class SN2ModTools : ModuleRules
{
	public SN2ModTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AssetTools",
			"AssetRegistry",
			"ContentBrowser",
			"ContentBrowserData",
			"DeveloperSettings",
			"ToolMenus",
			"Slate",
			"SlateCore",
			"InputCore",
			"DesktopPlatform",
			"Kismet",
			"UMGEditor",
			"UMG",
			"Settings",
		});
	}
}
