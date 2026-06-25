// Copyright PsinaDev. All Rights Reserved.

#include "SGifPreview.h"
#include "GifAsset.h"
#include "GifPlayer.h"
#include "SAnimatedGifImage.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "AnimatedGif"

void SGifPreview::Construct(const FArguments& InArgs, UGifAsset* InAsset, UGifPlayer* InPlayer)
{
	GifAsset = InAsset;
	GifPlayer = InPlayer;

	if (InPlayer)
	{
		// Repaint (and refresh scrubber/readout) whenever the displayed frame changes.
		InPlayer->OnFrameChangedNative.AddSP(this, &SGifPreview::HandleFrameChanged);
	}

	GifImage = SNew(SAnimatedGifImage);
	if (InPlayer)
	{
		const FIntPoint Dims = InPlayer->GetDimensions();
		GifImage->SetTexture(InPlayer->GetTexture(), FVector2D(Dims.X, Dims.Y));
	}
	// Keep the preview repainting (the asset-editor tab is otherwise idle and would
	// not pick up the per-frame texture updates).
	GifImage->SetPlaying(true);

	ChildSlot
	[
		SNew(SOverlay)

		// Checkerboard background (shows GIF transparency), fills the pane.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Checkerboard"))
		]

		// The GIF, aspect-correct, centered.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(8.0f, 8.0f, 8.0f, 40.0f) // leave room for the transport strip
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::Both)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				GifImage.ToSharedRef()
			]
		]

		// Transport strip pinned to the bottom.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(6.0f, 4.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SAssignNew(Scrubber, SSlider)
					.IsEnabled_Lambda([this]() { return GifPlayer.IsValid() && GifPlayer->GetNumFrames() > 1; })
					.Value_Raw(this, &SGifPreview::GetScrubValue)
					.OnMouseCaptureBegin_Raw(this, &SGifPreview::OnScrubBegin)
					.OnMouseCaptureEnd_Raw(this, &SGifPreview::OnScrubEnd)
					.OnValueChanged_Raw(this, &SGifPreview::OnScrubValueChanged)
					.ToolTipText(LOCTEXT("ScrubTip", "Scrub frames"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(10.0f, 0.0f, 2.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Raw(this, &SGifPreview::GetReadoutText)
				]
			]
		]
	];
}

void SGifPreview::RefreshTexture()
{
	if (GifImage.IsValid() && GifPlayer.IsValid())
	{
		const FIntPoint Dims = GifPlayer->GetDimensions();
		GifImage->SetTexture(GifPlayer->GetTexture(), FVector2D(Dims.X, Dims.Y));
		GifImage->SetPlaying(true);
	}
}

void SGifPreview::HandleFrameChanged()
{
	if (GifImage.IsValid())
	{
		GifImage->InvalidateFrame();
	}
	// Refresh the scrubber thumb and readout (their attributes re-evaluate on paint).
	Invalidate(EInvalidateWidgetReason::Paint);
}

float SGifPreview::GetScrubValue() const
{
	if (!GifPlayer.IsValid() || GifPlayer->GetNumFrames() <= 1)
	{
		return 0.0f;
	}
	if (Scrubber.IsValid() && Scrubber->HasMouseCapture())
	{
		return ScrubValue; // show the dragged position, not the live frame
	}
	return static_cast<float>(GifPlayer->GetCurrentFrame()) / static_cast<float>(GifPlayer->GetNumFrames() - 1);
}

void SGifPreview::OnScrubBegin()
{
	if (GifPlayer.IsValid())
	{
		bWasPlayingBeforeScrub = GifPlayer->IsPlaying();
		if (bWasPlayingBeforeScrub)
		{
			GifPlayer->Pause();
		}
	}
}

void SGifPreview::OnScrubValueChanged(float NewValue)
{
	ScrubValue = NewValue;
	if (GifPlayer.IsValid() && Scrubber.IsValid() && Scrubber->HasMouseCapture() && GifPlayer->GetNumFrames() > 1)
	{
		GifPlayer->SeekToFrame(FMath::RoundToInt(NewValue * (GifPlayer->GetNumFrames() - 1)));
	}
}

void SGifPreview::OnScrubEnd()
{
	if (GifPlayer.IsValid())
	{
		if (GifPlayer->GetNumFrames() > 1)
		{
			GifPlayer->SeekToFrame(FMath::RoundToInt(ScrubValue * (GifPlayer->GetNumFrames() - 1)));
		}
		if (bWasPlayingBeforeScrub)
		{
			GifPlayer->Play();
		}
	}
}

FText SGifPreview::GetReadoutText() const
{
	if (!GifPlayer.IsValid())
	{
		return FText::GetEmpty();
	}

	const int32 Current = GifPlayer->GetCurrentFrame();
	const int32 Total = GifPlayer->GetNumFrames();

	float Elapsed = 0.0f;
	float Duration = 0.0f;
	if (const UGifAsset* Asset = GifAsset.Get())
	{
		for (int32 i = 0; i < Asset->FrameDelays.Num(); ++i)
		{
			if (i <= Current)
			{
				Elapsed += Asset->FrameDelays[i];
			}
			Duration += Asset->FrameDelays[i];
		}
	}

	return FText::FromString(FString::Printf(TEXT("Frame %d / %d   ·   %.2fs / %.2fs"),
		Current + 1, Total, Elapsed, Duration));
}

#undef LOCTEXT_NAMESPACE
