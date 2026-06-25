// Copyright PsinaDev. All Rights Reserved.

#include "AnimatedGifImage.h"
#include "SAnimatedGifImage.h"
#include "GifAsset.h"
#include "GifPlayer.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimatedGifImage)

#define LOCTEXT_NAMESPACE "AnimatedGif"

TSharedRef<SWidget> UAnimatedGifImage::RebuildWidget()
{
	MyGif = SNew(SAnimatedGifImage)
		.ColorAndOpacity(ColorAndOpacity);
	return MyGif.ToSharedRef();
}

void UAnimatedGifImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RebuildPlayer();
}

void UAnimatedGifImage::RebuildPlayer()
{
	if (!MyGif.IsValid())
	{
		return;
	}

	// Only recreate the player (and its texture) when the asset actually changed.
	if (GifAsset != BuiltAsset)
	{
		if (Player)
		{
			Player->Stop();
			Player->OnFrameChangedNative.RemoveAll(this);
			Player = nullptr;
		}

		if (GifAsset)
		{
			Player = NewObject<UGifPlayer>(this);
			Player->Initialize(this, GifAsset->GetFrameSource());
			Player->OnFrameChangedNative.AddUObject(this, &UAnimatedGifImage::HandleFrameChanged);

			const FIntPoint Dim = Player->GetDimensions();
			MyGif->SetTexture(Player->GetTexture(), FVector2D(Dim.X, Dim.Y));
		}
		else
		{
			MyGif->SetTexture(nullptr, FVector2D::ZeroVector);
		}

		BuiltAsset = GifAsset;
	}

	if (Player)
	{
		Player->bLooping = bLooping;
		Player->PlayRate = PlayRate;
		if (LoopCountOverride >= 0)
		{
			Player->LoopCount = LoopCountOverride;
		}

		// Decide play state here (not just on asset change) so toggling bAutoPlay /
		// bPreviewInDesigner in the editor takes effect without recreating the player.
		bool bShouldPlay = bAutoPlay;
#if WITH_EDITORONLY_DATA
		if (IsDesignTime() && !bPreviewInDesigner)
		{
			bShouldPlay = false;
		}
#endif
		if (bShouldPlay && !Player->IsPlaying())
		{
			Player->Play();
		}
		else if (!bShouldPlay && Player->IsPlaying())
		{
			Player->Pause();
		}

		// Drive (or stop) the Slate repaint timer to match the play state.
		MyGif->SetPlaying(bShouldPlay && Player->GetNumFrames() > 1);
	}
	else
	{
		MyGif->SetPlaying(false);
	}
	MyGif->SetColorAndOpacity(ColorAndOpacity);
}

void UAnimatedGifImage::HandleFrameChanged()
{
	if (MyGif.IsValid())
	{
		MyGif->InvalidateFrame();
	}
}

void UAnimatedGifImage::SetGifAsset(UGifAsset* InAsset)
{
	GifAsset = InAsset;
	RebuildPlayer();
}

void UAnimatedGifImage::Play()
{
	if (Player)
	{
		Player->Play();
	}
}

void UAnimatedGifImage::Pause()
{
	if (Player)
	{
		Player->Pause();
	}
}

void UAnimatedGifImage::Stop()
{
	if (Player)
	{
		Player->Stop();
	}
}

void UAnimatedGifImage::SetColorAndOpacity(FLinearColor InColor)
{
	ColorAndOpacity = InColor;
	if (MyGif.IsValid())
	{
		MyGif->SetColorAndOpacity(InColor);
	}
}

void UAnimatedGifImage::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	if (Player)
	{
		Player->Stop();
		Player->OnFrameChangedNative.RemoveAll(this);
		Player = nullptr;
	}
	BuiltAsset = nullptr;
	MyGif.Reset();
}

#if WITH_EDITOR
const FText UAnimatedGifImage::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "Image");
}
#endif

#undef LOCTEXT_NAMESPACE
