// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Internationalization/Text.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Widgets/SCompoundWidget.h"

class UGifAsset;
class UGifPlayer;
class SAnimatedGifImage;
class SSlider;

/**
 * Preview pane for the GIF editor: a checkerboard background, the GIF scaled-to-fit
 * and centered, and a bottom transport strip (scrub slider + frame/time readout).
 * Repaints only when the player reports a new frame.
 */
class SGifPreview : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGifPreview) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UGifAsset* InAsset, UGifPlayer* InPlayer);

	/** Re-point the preview at the player's current output texture (e.g. after the sink switched RT<->internal). */
	void RefreshTexture();

private:
	void HandleFrameChanged();

	float GetScrubValue() const;
	void OnScrubBegin();
	void OnScrubEnd();
	void OnScrubValueChanged(float NewValue);

	FText GetReadoutText() const;

	TWeakObjectPtr<UGifAsset> GifAsset;
	TWeakObjectPtr<UGifPlayer> GifPlayer;

	TSharedPtr<SAnimatedGifImage> GifImage;
	TSharedPtr<SSlider> Scrubber;

	float ScrubValue = 0.0f;
	bool bWasPlayingBeforeScrub = false;
};
