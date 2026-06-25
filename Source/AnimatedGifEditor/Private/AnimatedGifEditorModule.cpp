// Copyright PsinaDev. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "GifAsset.h"
#include "AnimatedGifThumbnailRenderer.h"
#include "GifAssetEditorCommands.h"

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Brushes/SlateImageBrush.h"
#include "Misc/Paths.h"

class FAnimatedGifEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UThumbnailManager::Get().RegisterCustomRenderer(UGifAsset::StaticClass(), UAnimatedGifThumbnailRenderer::StaticClass());
		RegisterStyle();
	}

	virtual void ShutdownModule() override
	{
		FGifAssetEditorCommands::Unregister();
		UnregisterStyle();
		if (UObjectInitialized())
		{
			UThumbnailManager::Get().UnregisterCustomRenderer(UGifAsset::StaticClass());
		}
	}

private:
	/** Give UAnimatedGifImage the stock UImage palette/hierarchy icon by aliasing UMG's Image SVGs. */
	void RegisterStyle()
	{
		Style = MakeShared<FSlateStyleSet>("AnimatedGifEditorStyle");

		const FString UMGSlate = FPaths::EngineContentDir() / TEXT("Editor/Slate/UMG");
		Style->Set("ClassIcon.AnimatedGifImage",
			new FSlateVectorImageBrush(UMGSlate / TEXT("Image.svg"), FVector2D(16.0f, 16.0f)));
		Style->Set("ClassThumbnail.AnimatedGifImage",
			new FSlateVectorImageBrush(UMGSlate / TEXT("Image_64.svg"), FVector2D(64.0f, 64.0f)));

		FSlateStyleRegistry::RegisterSlateStyle(*Style);
	}

	void UnregisterStyle()
	{
		if (Style.IsValid())
		{
			FSlateStyleRegistry::UnRegisterSlateStyle(*Style);
			Style.Reset();
		}
	}

	TSharedPtr<FSlateStyleSet> Style;
};

IMPLEMENT_MODULE(FAnimatedGifEditorModule, AnimatedGifEditor)
