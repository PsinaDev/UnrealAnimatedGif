// Copyright PsinaDev. All Rights Reserved.

using UnrealBuildTool;

public class AnimatedGifEditor : ModuleRules
{
	public AnimatedGifEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetDefinition",
			"ToolMenus",
			"PropertyEditor",
			"EditorStyle",
			"InputCore", 
			"RHI",
			"RenderCore",
			"Slate",
			"SlateCore",
			"AnimatedGif"
		});
	}
}
