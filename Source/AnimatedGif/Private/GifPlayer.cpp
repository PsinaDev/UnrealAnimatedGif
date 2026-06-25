// Copyright PsinaDev. All Rights Reserved.

#include "GifPlayer.h"
#include "AnimatedGifModule.h"
#include "AnimatedGifSettings.h"
#include "GifPlaybackSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2DDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "Misc/App.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(GifPlayer)

void UGifPlayer::Initialize(UObject* WorldContext, const FAnimatedImageDataRef& InSource)
{
	Source = InSource;
	World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	LoopCount = InSource->LoopCount; // seed from the asset/file; caller may override afterwards

	EnsureTexture();

	if (Source->GetNumFrames() > 0)
	{
		AccumulatedTime = 0.0f;
		SetCurrentFrame(0, /*bForceUpload*/ true);
	}
}

void UGifPlayer::EnsureTexture()
{
	if (!Source.IsValid())
	{
		return;
	}

	RebuildTiming();

	if (RenderTarget)
	{
		// An external render target is the sink; no internal texture needed.
		ConfigureRenderTarget();
		return;
	}

	if (!OutputTexture && Source->Width > 0 && Source->Height > 0)
	{
		// SRGB true -> hardware sRGB->linear on sample, correct in both Slate and materials.
		// Clamp addressing: GIFs are images, not tiling patterns.
		const FTexture2DDynamicCreateInfo CreateInfo(PF_B8G8R8A8, /*bIsResolveTarget*/ false, /*bSRGB*/ true, TF_Bilinear, AM_Clamp);
		OutputTexture = UTexture2DDynamic::Create(Source->Width, Source->Height, CreateInfo);
	}
}

void UGifPlayer::ConfigureRenderTarget()
{
	if (!RenderTarget || !Source.IsValid())
	{
		return;
	}

	// Match the GIF: BGRA8 + sRGB so our frame bytes sample correctly. Runtime-only
	// (the asset's saved settings are untouched on disk).
	if (RenderTarget->SizeX != Source->Width || RenderTarget->SizeY != Source->Height
		|| RenderTarget->GetFormat() != PF_B8G8R8A8)
	{
		RenderTarget->InitCustomFormat(Source->Width, Source->Height, PF_B8G8R8A8, /*bForceLinearGamma*/ false);
		RenderTarget->UpdateResourceImmediate(/*bClearRenderTarget*/ true);
	}
}

FTextureResource* UGifPlayer::GetActiveResource() const
{
	if (RenderTarget)
	{
		return RenderTarget->GameThread_GetRenderTargetResource();
	}
	return OutputTexture ? OutputTexture->GetResource() : nullptr;
}

void UGifPlayer::SetRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
	if (RenderTarget == InRenderTarget)
	{
		return;
	}

	RenderTarget = InRenderTarget;
	if (RenderTarget)
	{
		OutputTexture = nullptr; // the render target is now the sole sink
		ConfigureRenderTarget();
	}
	else
	{
		EnsureTexture(); // reverting to an internal texture
	}

	if (CurrentFrame != INDEX_NONE)
	{
		UploadFrameToGPU(CurrentFrame); // re-push the current frame into the new sink
	}
}

void UGifPlayer::RebuildTiming()
{
	CumulativeEndTimes.Reset();
	TotalDuration = 0.0f;

	if (!Source.IsValid())
	{
		return;
	}

	const UAnimatedGifSettings& Settings = UAnimatedGifSettings::Get();
	CumulativeEndTimes.Reserve(Source->GetNumFrames());
	for (float Raw : Source->FrameDelays)
	{
		const float Effective = (Raw < Settings.MinFrameDelaySeconds) ? Settings.FallbackFrameDelaySeconds : Raw;
		TotalDuration += Effective;
		CumulativeEndTimes.Add(TotalDuration);
	}
}

int32 UGifPlayer::FrameAtTime(float WrappedTime) const
{
	const int32 N = CumulativeEndTimes.Num();
	for (int32 i = 0; i < N; ++i)
	{
		if (WrappedTime < CumulativeEndTimes[i])
		{
			return i;
		}
	}
	return FMath::Max(0, N - 1);
}

void UGifPlayer::TickPlayer(float DriverDeltaSeconds)
{
	if (!bPlaying || !Source.IsValid())
	{
		return;
	}

	const int32 N = Source->GetNumFrames();
	if (N <= 1 || TotalDuration <= 0.0f)
	{
		bPlaying = false;
		UnregisterFromDriver();
		return;
	}

	if (const UWorld* W = World.Get())
	{
		if (W->IsPaused() && !bIgnorePause)
		{
			return;
		}
	}

	const float Delta = (bIgnoreTimeDilation ? FApp::GetDeltaTime() : DriverDeltaSeconds) * PlayRate;
	AccumulatedTime += Delta;

	bool bLoopedThisTick = false;
	while (AccumulatedTime >= TotalDuration)
	{
		AccumulatedTime -= TotalDuration;
		++LoopsCompleted;
		bLoopedThisTick = true;

		// bLooping=false -> play once; bLooping=true -> stop after LoopCount play-throughs (0 = forever).
		const bool bStop = !bLooping || (LoopCount > 0 && LoopsCompleted >= LoopCount);
		if (bStop)
		{
			SetCurrentFrame(N - 1, /*bForceUpload*/ false); // freeze on the final frame
			bPlaying = false;
			bFinished = true;
			UnregisterFromDriver();
			OnFinished.Broadcast();
			return;
		}
	}

	SetCurrentFrame(FrameAtTime(AccumulatedTime), /*bForceUpload*/ false);

	if (bLoopedThisTick)
	{
		OnLooped.Broadcast();
	}
}

void UGifPlayer::SetCurrentFrame(int32 NewFrame, bool bForceUpload)
{
	if (!Source.IsValid())
	{
		return;
	}

	NewFrame = FMath::Clamp(NewFrame, 0, Source->GetNumFrames() - 1);
	if (!bForceUpload && NewFrame == CurrentFrame)
	{
		return;
	}

	CurrentFrame = NewFrame;
	UploadFrameToGPU(NewFrame);
	OnFrameChangedNative.Broadcast();
}

void UGifPlayer::UploadFrameToGPU(int32 FrameIndex)
{
	if (!Source.IsValid())
	{
		return;
	}

	// Sink is either our UTexture2DDynamic or a linked render target; both expose the
	// live RHI texture via FTextureResource::TextureRHI.
	FTextureResource* Resource = GetActiveResource();
	if (!Resource)
	{
		return;
	}

	const int32 W = Source->Width;
	const int32 H = Source->Height;
	const uint8* FrameData = Source->GetFrameData(FrameIndex);
	FAnimatedImageDataPtr KeepAlive = Source; // hold frames alive until the GPU upload runs

	ENQUEUE_RENDER_COMMAND(UploadGifFrame)(
		[Resource, FrameData, W, H, KeepAlive](FRHICommandListImmediate& RHICmdList)
		{
			const FTextureRHIRef TextureRHI = Resource->TextureRHI; // hold ref for the update
			if (!TextureRHI.IsValid()
				|| static_cast<int32>(TextureRHI->GetSizeX()) < W
				|| static_cast<int32>(TextureRHI->GetSizeY()) < H)
			{
				return; // not initialized yet, or sink smaller than the frame
			}
			const FUpdateTextureRegion2D Region(0, 0, 0, 0, W, H);
			RHICmdList.UpdateTexture2D(TextureRHI, 0, Region, static_cast<uint32>(W) * 4u, FrameData);
		});
}

void UGifPlayer::Play()
{
	if (!Source.IsValid())
	{
		return;
	}

	EnsureTexture();

	if (Source->GetNumFrames() <= 1)
	{
		SetCurrentFrame(0, /*bForceUpload*/ true); // single-frame GIF is static
		return;
	}

	if (bFinished)
	{
		// Restart a finished animation rather than resuming at the end.
		AccumulatedTime = 0.0f;
		LoopsCompleted = 0;
		bFinished = false;
		SetCurrentFrame(0, /*bForceUpload*/ true);
	}

	bPlaying = true;
	RegisterWithDriver();
}

void UGifPlayer::Pause()
{
	bPlaying = false;
	UnregisterFromDriver();
}

void UGifPlayer::Stop()
{
	bPlaying = false;
	bFinished = false;
	UnregisterFromDriver();
	AccumulatedTime = 0.0f;
	LoopsCompleted = 0;
	SetCurrentFrame(0, /*bForceUpload*/ true);
}

void UGifPlayer::SeekToFrame(int32 FrameIndex)
{
	if (!Source.IsValid() || CumulativeEndTimes.Num() == 0)
	{
		return;
	}

	FrameIndex = FMath::Clamp(FrameIndex, 0, Source->GetNumFrames() - 1);
	AccumulatedTime = (FrameIndex == 0) ? 0.0f : CumulativeEndTimes[FrameIndex - 1];
	LoopsCompleted = 0;
	bFinished = false;
	SetCurrentFrame(FrameIndex, /*bForceUpload*/ true);
}

void UGifPlayer::SeekToTime(float TimeSeconds)
{
	if (!Source.IsValid() || TotalDuration <= 0.0f)
	{
		return;
	}

	AccumulatedTime = FMath::Fmod(FMath::Max(0.0f, TimeSeconds), TotalDuration);
	LoopsCompleted = 0;
	bFinished = false;
	SetCurrentFrame(FrameAtTime(AccumulatedTime), /*bForceUpload*/ true);
}

UTexture* UGifPlayer::GetTexture() const
{
	if (RenderTarget)
	{
		return RenderTarget;
	}
	return OutputTexture;
}

FIntPoint UGifPlayer::GetDimensions() const
{
	return Source.IsValid() ? FIntPoint(Source->Width, Source->Height) : FIntPoint::ZeroValue;
}

void UGifPlayer::RegisterWithDriver()
{
	if (bRegistered)
	{
		return;
	}

	if (UWorld* W = World.Get())
	{
		if (UGifPlaybackSubsystem* Subsystem = W->GetSubsystem<UGifPlaybackSubsystem>())
		{
			Subsystem->RegisterPlayer(this);
			bUsesGlobalTicker = false;
			bRegistered = true;
			return;
		}
	}

	// No world (e.g. loading screen) -> real-time global ticker.
	FGifGlobalTicker::Get().Register(this);
	bUsesGlobalTicker = true;
	bRegistered = true;
}

void UGifPlayer::UnregisterFromDriver()
{
	if (!bRegistered)
	{
		return;
	}

	if (bUsesGlobalTicker)
	{
		FGifGlobalTicker::Get().Unregister(this);
	}
	else if (UWorld* W = World.Get())
	{
		if (UGifPlaybackSubsystem* Subsystem = W->GetSubsystem<UGifPlaybackSubsystem>())
		{
			Subsystem->UnregisterPlayer(this);
		}
	}

	bRegistered = false;
}

void UGifPlayer::BeginDestroy()
{
	UnregisterFromDriver();
	Super::BeginDestroy();
}
