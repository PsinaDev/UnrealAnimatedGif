// Copyright PsinaDev. All Rights Reserved.

#include "AnimatedGifThumbnailRenderer.h"
#include "GifAsset.h"
#include "AnimatedImageTypes.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "SceneTypes.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "RHITypes.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimatedGifThumbnailRenderer)

bool UAnimatedGifThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	const UGifAsset* Gif = Cast<UGifAsset>(Object);
	return Gif && Gif->GetNumFrames() > 0;
}

void UAnimatedGifThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const
{
	if (const UGifAsset* Gif = Cast<UGifAsset>(Object))
	{
		OutWidth = FMath::TruncToInt(Zoom * Gif->Width);
		OutHeight = FMath::TruncToInt(Zoom * Gif->Height);
	}
	else
	{
		OutWidth = 0;
		OutHeight = 0;
	}
}

void UAnimatedGifThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget,
	FCanvas* Canvas, bool bAdditionalViewFamily)
{
#if WITH_EDITORONLY_DATA
	UGifAsset* Gif = Cast<UGifAsset>(Object);
	if (!Gif)
	{
		return;
	}

	UTexture2D* Texture = Gif->GetThumbnailTexture();
	if (!Texture || !Texture->GetResource())
	{
		return; // resource not ready yet; the thumbnail pool will redraw
	}

	// Advance to the frame for the current wall-clock time and push it into the tile texture.
	const FAnimatedImageDataRef Src = Gif->GetFrameSource();
	if (Src->IsValid() && Gif->GetNumFrames() > 1)
	{
		const float TotalDuration = Gif->GetDurationSeconds();
		int32 FrameIndex = 0;
		if (TotalDuration > UE_SMALL_NUMBER)
		{
			float T = FMath::Fmod(static_cast<float>(UThumbnailRenderer::GetTime().GetRealTimeSeconds()), TotalDuration);
			for (int32 i = 0; i < Gif->GetNumFrames(); ++i)
			{
				const float Delay = Gif->FrameDelays[i];
				FrameIndex = i;
				if (T < Delay)
				{
					break;
				}
				T -= Delay;
			}
		}

		// Upload the frame on the render thread. The frame bytes outlive this call
		// because the asset's cached source owns them for its lifetime. Same path as
		// UGifPlayer::UploadFrameToGPU.
		FTextureResource* Resource = Texture->GetResource();
		const int32 W = Src->Width;
		const int32 H = Src->Height;
		const uint8* FramePtr = Src->GetFrameData(FrameIndex);
		ENQUEUE_RENDER_COMMAND(UploadGifThumbnail)(
			[Resource, FramePtr, W, H](FRHICommandListImmediate& RHICmdList)
			{
				const FTextureRHIRef TextureRHI = Resource->TextureRHI;
				if (!TextureRHI.IsValid()
					|| static_cast<int32>(TextureRHI->GetSizeX()) < W
					|| static_cast<int32>(TextureRHI->GetSizeY()) < H)
				{
					return;
				}
				const FUpdateTextureRegion2D Region(0, 0, 0, 0, W, H);
				RHICmdList.UpdateTexture2D(TextureRHI, 0, Region, static_cast<uint32>(W) * 4u, FramePtr);
			});
	}

	FCanvasTileItem TileItem(FVector2D(X, Y), Texture->GetResource(), FVector2D(Width, Height), FLinearColor::White);
	TileItem.BlendMode = SE_BLEND_Translucent; // honour GIF transparency
	Canvas->DrawItem(TileItem);
#endif
}
