// Copyright PsinaDev. All Rights Reserved.

#include "SAnimatedGifImage.h"
#include "Slate/DeferredCleanupSlateBrush.h"
#include "Styling/SlateBrush.h"
#include "Rendering/DrawElements.h"

void SAnimatedGifImage::Construct(const FArguments& InArgs)
{
	ColorAndOpacity = InArgs._ColorAndOpacity;
	SetCanTick(false); // repaint is event-driven (InvalidateFrame), not per-frame
}

void SAnimatedGifImage::SetTexture(UTexture* InTexture, const FVector2D& InImageSize)
{
	ImageSize = InImageSize;
	if (InTexture)
	{
		Brush = FDeferredCleanupSlateBrush::CreateBrush(InTexture); // TSharedRef -> TSharedPtr
	}
	else
	{
		Brush.Reset();
	}

	Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
}

void SAnimatedGifImage::SetColorAndOpacity(const FLinearColor& InColor)
{
	if (!ColorAndOpacity.Equals(InColor))
	{
		ColorAndOpacity = InColor;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SAnimatedGifImage::InvalidateFrame()
{
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SAnimatedGifImage::SetPlaying(bool bInPlaying)
{
	if (bInPlaying)
	{
		if (!AnimationTimer.IsValid())
		{
			AnimationTimer = RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SAnimatedGifImage::ActiveTick));
		}
	}
	else if (AnimationTimer.IsValid())
	{
		UnRegisterActiveTimer(AnimationTimer.ToSharedRef());
		AnimationTimer.Reset();
		Invalidate(EInvalidateWidgetReason::Paint); // settle on the final frame
	}
}

EActiveTimerReturnType SAnimatedGifImage::ActiveTick(double InCurrentTime, float InDeltaTime)
{
	// Keeps the containing window awake and repainting while the GIF plays.
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

int32 SAnimatedGifImage::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FSlateBrush* SlateBrush = FDeferredCleanupSlateBrush::TrySlateBrush(Brush);
	if (SlateBrush && SlateBrush->GetResourceObject())
	{
		const bool bEnabled = ShouldBeEnabled(bParentEnabled);
		const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		const FLinearColor FinalTint = ColorAndOpacity * InWidgetStyle.GetColorAndOpacityTint();

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			SlateBrush,
			DrawEffects,
			FinalTint);
	}

	return LayerId;
}

FVector2D SAnimatedGifImage::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return ImageSize;
}
