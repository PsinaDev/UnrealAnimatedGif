// Copyright PsinaDev. All Rights Reserved.

using UnrealBuildTool;

public class AnimatedGif : ModuleRules
{
	public AnimatedGif(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"Slate",
			"SlateCore",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RHI",
			"RenderCore",
			"stb_image"
		});
	}
}
