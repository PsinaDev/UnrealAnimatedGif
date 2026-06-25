// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Math/Color.h"
#include "Math/Vector2D.h"
#include "Templates/SharedPointer.h"
#include "Widgets/SLeafWidget.h"

class FDeferredCleanupSlateBrush;
class UTexture;

/**
 * Leaf widget that draws a UGifPlayer's live texture. The texture object is stable
 * across frames (only its texels change), so we only repaint when told to via
 * InvalidateFrame() — driven by the player's frame-change event, not a polling tick.
 */
class ANIMATEDGIF_API SAnimatedGifImage : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimatedGifImage)
		: _ColorAndOpacity(FLinearColor::White)
	{}
		SLATE_ARGUMENT(FLinearColor, ColorAndOpacity)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Point the widget at a texture; ImageSize becomes the natural desired size. */
	void SetTexture(UTexture* InTexture, const FVector2D& InImageSize);
	void SetColorAndOpacity(const FLinearColor& InColor);

	/** Request a repaint because the texture's texels changed. */
	void InvalidateFrame();

	/**
	 * While playing, run a Slate active timer that repaints every frame. This is what
	 * actually animates the widget: the texture's texels change on the GPU, but an idle
	 * window (e.g. an asset-editor tab) won't redraw on Paint-invalidation alone — the
	 * active timer keeps the window awake and repainting. Off when paused/static.
	 */
	void SetPlaying(bool bInPlaying);

	//~ SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	EActiveTimerReturnType ActiveTick(double InCurrentTime, float InDeltaTime);

	TSharedPtr<FDeferredCleanupSlateBrush> Brush;
	TSharedPtr<FActiveTimerHandle> AnimationTimer;
	FVector2D ImageSize = FVector2D::ZeroVector;
	FLinearColor ColorAndOpacity = FLinearColor::White;
};
