// Copyright PsinaDev. All Rights Reserved.

using UnrealBuildTool;

// Header-only vendored decoder (public domain / MIT). We only expose the include
// path here; STB_IMAGE_IMPLEMENTATION is compiled in a single TU inside the
// AnimatedGif runtime module (see StbImageImpl.cpp).
public class stb_image : ModuleRules
{
	public stb_image(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicSystemIncludePaths.Add(ModuleDirectory);
	}
}
