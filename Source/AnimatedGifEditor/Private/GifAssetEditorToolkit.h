// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class UGifAsset;
class UGifPlayer;
class IDetailsView;
class SGifPreview;
class SDockTab;
class FSpawnTabArgs;
class FToolBarBuilder;

/**
 * Standalone editor for UGifAsset: a checkerboard preview viewport with a transport
 * strip on the left, a Details panel on the right, and a toolbar with Play/Pause/Stop/
 * Loop. The preview is driven by a UGifPlayer with no world, so it ticks on the
 * in-editor FGifGlobalTicker. The player and asset are GC-rooted via FGCObject.
 */
class FGifAssetEditorToolkit : public FAssetEditorToolkit, public FGCObject
{
public:
	static TSharedRef<FGifAssetEditorToolkit> CreateEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, UGifAsset* Asset);

	virtual ~FGifAssetEditorToolkit() override;

	void InitGifEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, UGifAsset* Asset);

	//~ FAssetEditorToolkit
	virtual FName GetToolkitFName() const override { return TEXT("GifAssetEditor"); }
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("GIF "); }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.6f, 0.3f, 0.7f, 0.5f); }
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	//~ FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FGifAssetEditorToolkit"); }

private:
	void CreateInternalWidgets();
	void HandleAssetPropertyChanged(const struct FPropertyChangedEvent& Event);
	void BindCommands();
	void ExtendToolBar();
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);

	TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);

	// Command handlers
	void HandlePlay();
	void HandlePause();
	void HandleStop();
	void HandleToggleLoop();
	bool CanPlay() const;
	bool CanPause() const;
	bool IsLooping() const;

	static const FName ViewportTabId;
	static const FName DetailsTabId;

	TObjectPtr<UGifAsset> GifAsset = nullptr;
	TObjectPtr<UGifPlayer> PreviewPlayer = nullptr; // rooted via AddReferencedObjects

	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SGifPreview> PreviewWidget;
};
