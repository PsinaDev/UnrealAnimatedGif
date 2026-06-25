// Copyright PsinaDev. All Rights Reserved.

#include "GifAssetEditorToolkit.h"
#include "GifAssetEditorCommands.h"
#include "GifAsset.h"
#include "GifPlayer.h"
#include "SGifPreview.h"

#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"

#define LOCTEXT_NAMESPACE "AnimatedGif"

const FName FGifAssetEditorToolkit::ViewportTabId(TEXT("GifAssetEditor_Viewport"));
const FName FGifAssetEditorToolkit::DetailsTabId(TEXT("GifAssetEditor_Details"));

TSharedRef<FGifAssetEditorToolkit> FGifAssetEditorToolkit::CreateEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, UGifAsset* Asset)
{
	TSharedRef<FGifAssetEditorToolkit> Editor = MakeShared<FGifAssetEditorToolkit>();
	Editor->InitGifEditor(Mode, Host, Asset);
	return Editor;
}

FGifAssetEditorToolkit::~FGifAssetEditorToolkit()
{
	if (PreviewPlayer)
	{
		PreviewPlayer->Stop(); // unregister from the editor ticker before we stop rooting it
	}
}

FText FGifAssetEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "GIF Editor");
}

void FGifAssetEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(GifAsset);       // DetailsView holds a raw pointer to it
	Collector.AddReferencedObject(PreviewPlayer);
}

void FGifAssetEditorToolkit::InitGifEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, UGifAsset* Asset)
{
	GifAsset = Asset;

	if (Asset)
	{
		// No world context -> the player ticks on FGifGlobalTicker (runs in-editor).
		PreviewPlayer = NewObject<UGifPlayer>(GetTransientPackage());
		PreviewPlayer->bLooping = true;
		// If the asset links a render target, drive it from the preview so it updates
		// live in the editor (like a SceneCapture target) — the preview pane then shows
		// that same render target. Otherwise, the player uses its own internal texture.
		if (Asset->LinkedRenderTarget)
		{
			PreviewPlayer->SetRenderTarget(Asset->LinkedRenderTarget);
		}
		PreviewPlayer->Initialize(nullptr, Asset->GetFrameSource());
		PreviewPlayer->Play();
	}

	FGifAssetEditorCommands::Register();
	BindCommands();
	CreateInternalWidgets();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("GifAssetEditor_Layout_v2")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->AddTab(ViewportTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
				->SetSizeCoefficient(0.7f)
			)
			->Split(
				FTabManager::NewStack()
				->AddTab(DetailsTabId, ETabState::OpenedTab)
				->SetSizeCoefficient(0.3f)
			)
		);

	FAssetEditorToolkit::InitAssetEditor(Mode, Host, TEXT("GifAssetEditorApp"), Layout,
		/*bCreateDefaultStandaloneMenu*/ true, /*bCreateDefaultToolbar*/ true, Asset);

	ExtendToolBar();
	RegenerateMenusAndToolbars();
}

void FGifAssetEditorToolkit::CreateInternalWidgets()
{
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = PropertyModule.CreateDetailView(DetailsArgs);
	DetailsView->SetObject(GifAsset);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FGifAssetEditorToolkit::HandleAssetPropertyChanged);

	PreviewWidget = SNew(SGifPreview, GifAsset.Get(), PreviewPlayer.Get());
}

void FGifAssetEditorToolkit::HandleAssetPropertyChanged(const FPropertyChangedEvent& Event)
{
	if (!PreviewPlayer || !GifAsset)
	{
		return;
	}

	// Keep the live preview in sync with edits in the Details panel: re-point the linked
	// render target (set/changed/cleared — clearing stops the old RT being overwritten)
	// and re-apply the loop count.
	PreviewPlayer->LoopCount = GifAsset->LoopCount;
	PreviewPlayer->SetRenderTarget(GifAsset->LinkedRenderTarget);

	if (PreviewWidget.IsValid())
	{
		PreviewWidget->RefreshTexture();
	}
}

void FGifAssetEditorToolkit::BindCommands()
{
	const FGifAssetEditorCommands& Commands = FGifAssetEditorCommands::Get();
	const TSharedRef<FUICommandList> UICommands = GetToolkitCommands();

	UICommands->MapAction(Commands.Play,
		FExecuteAction::CreateSP(this, &FGifAssetEditorToolkit::HandlePlay),
		FCanExecuteAction::CreateSP(this, &FGifAssetEditorToolkit::CanPlay));

	UICommands->MapAction(Commands.Pause,
		FExecuteAction::CreateSP(this, &FGifAssetEditorToolkit::HandlePause),
		FCanExecuteAction::CreateSP(this, &FGifAssetEditorToolkit::CanPause));

	UICommands->MapAction(Commands.Stop,
		FExecuteAction::CreateSP(this, &FGifAssetEditorToolkit::HandleStop));

	UICommands->MapAction(Commands.ToggleLoop,
		FExecuteAction::CreateSP(this, &FGifAssetEditorToolkit::HandleToggleLoop),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FGifAssetEditorToolkit::IsLooping));
}

void FGifAssetEditorToolkit::ExtendToolBar()
{
	const TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FGifAssetEditorToolkit::FillToolbar));
	AddToolbarExtender(ToolbarExtender);
}

void FGifAssetEditorToolkit::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	const FName StyleName = FAppStyle::GetAppStyleSetName();
	const FGifAssetEditorCommands& Commands = FGifAssetEditorCommands::Get();

	ToolbarBuilder.BeginSection("Playback");
	{
		ToolbarBuilder.AddToolBarButton(Commands.Play, NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			FSlateIcon(StyleName, "Animation.Forward"));
		ToolbarBuilder.AddToolBarButton(Commands.Pause, NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			FSlateIcon(StyleName, "Animation.Pause"));
		ToolbarBuilder.AddToolBarButton(Commands.Stop, NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			FSlateIcon(StyleName, "Animation.Stop"));
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Options");
	{
		ToolbarBuilder.AddToolBarButton(Commands.ToggleLoop, NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			FSlateIcon(StyleName, "Animation.Loop.Enabled"));
	}
	ToolbarBuilder.EndSection();
}

TSharedRef<SDockTab> FGifAssetEditorToolkit::SpawnViewportTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTabLabel", "Preview"))
		[
			PreviewWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FGifAssetEditorToolkit::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

void FGifAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu", "GIF Editor"));
	const TSharedRef<FWorkspaceItem> Category = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FGifAssetEditorToolkit::SpawnViewportTab))
		.SetDisplayName(LOCTEXT("ViewportTab", "Preview"))
		.SetGroup(Category)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FGifAssetEditorToolkit::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(Category)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FGifAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
}

void FGifAssetEditorToolkit::HandlePlay() { if (PreviewPlayer) { PreviewPlayer->Play(); } }
void FGifAssetEditorToolkit::HandlePause() { if (PreviewPlayer) { PreviewPlayer->Pause(); } }
void FGifAssetEditorToolkit::HandleStop() { if (PreviewPlayer) { PreviewPlayer->Stop(); } }
void FGifAssetEditorToolkit::HandleToggleLoop() { if (PreviewPlayer) { PreviewPlayer->bLooping = !PreviewPlayer->bLooping; } }
bool FGifAssetEditorToolkit::CanPlay() const { return PreviewPlayer && !PreviewPlayer->IsPlaying(); }
bool FGifAssetEditorToolkit::CanPause() const { return PreviewPlayer && PreviewPlayer->IsPlaying(); }
bool FGifAssetEditorToolkit::IsLooping() const { return PreviewPlayer && PreviewPlayer->bLooping; }

#undef LOCTEXT_NAMESPACE
