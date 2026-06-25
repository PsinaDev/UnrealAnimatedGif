// Copyright PsinaDev. All Rights Reserved.

#include "AssetDefinition_AnimatedGif.h"
#include "GifAsset.h"
#include "GifAssetEditorToolkit.h"
#include "AssetDefinition.h" // EAssetCategoryPaths + FAssetCategoryPath
#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_AnimatedGif)

#define LOCTEXT_NAMESPACE "AnimatedGif"

FText UAssetDefinition_AnimatedGif::GetAssetDisplayName() const
{
	return LOCTEXT("AssetDisplayName", "Animated GIF");
}

TSoftClassPtr<> UAssetDefinition_AnimatedGif::GetAssetClass() const
{
	return UGifAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_AnimatedGif::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Texture };
	return Categories;
}

EAssetCommandResult UAssetDefinition_AnimatedGif::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UGifAsset* Asset : OpenArgs.LoadObjects<UGifAsset>())
	{
		if (Asset)
		{
			FGifAssetEditorToolkit::CreateEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Asset);
		}
	}
	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
