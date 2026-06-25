// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "CoreTypes.h"
#include "Delegates/Delegate.h"
#include "Engine/World.h"
#include "Math/IntPoint.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "AnimatedImageTypes.h"
#include "GifPlayer.generated.h"

class UTexture;
class UTexture2DDynamic;
class UTextureRenderTarget2D;
class FTextureResource;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGifPlaybackEvent);

/**
 * Per-instance playhead + GPU sink. Owns one UTexture2DDynamic and advances a
 * playhead over a shared FAnimatedImageData; on each frame change it uploads the
 * frame to the GPU. Created either from a UGifAsset or from a runtime decode.
 */
UCLASS(BlueprintType)
class ANIMATEDGIF_API UGifPlayer : public UObject
{
	GENERATED_BODY()

public:
	/** Bind to a frame source and create the output texture (uploads frame 0). */
	void Initialize(UObject* WorldContext, const FAnimatedImageDataRef& InSource);

	UFUNCTION(BlueprintCallable, Category = "GIF")
	void Play();

	UFUNCTION(BlueprintCallable, Category = "GIF")
	void Pause();

	/** Stop and rewind to the first frame. */
	UFUNCTION(BlueprintCallable, Category = "GIF")
	void Stop();

	UFUNCTION(BlueprintCallable, Category = "GIF")
	void SeekToFrame(int32 FrameIndex);

	UFUNCTION(BlueprintCallable, Category = "GIF")
	void SeekToTime(float TimeSeconds);

	/** The live, self-updating texture (UTexture2DDynamic or render target). Bind to a material, or to a UImage via SetBrushResourceObject. */
	UFUNCTION(BlueprintPure, Category = "GIF")
	UTexture* GetTexture() const;

	/** Internal dynamic texture, typed for UImage::SetBrushFromTextureDynamic. Null when a render target is the sink. */
	UFUNCTION(BlueprintPure, Category = "GIF")
	UTexture2DDynamic* GetDynamicTexture() const { return OutputTexture; }

	/** The render target this player drives, if one is set (e.g. the asset's linked target). */
	UFUNCTION(BlueprintPure, Category = "GIF")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/**
	 * Drive a render-target asset instead of an internal texture, so the GIF can be
	 * sampled in a material graph. The target is reconfigured to this GIF's size and
	 * BGRA8 format. Pass null to revert to an internal texture. Best set before Play().
	 */
	UFUNCTION(BlueprintCallable, Category = "GIF")
	void SetRenderTarget(UTextureRenderTarget2D* InRenderTarget);

	UFUNCTION(BlueprintPure, Category = "GIF")
	int32 GetCurrentFrame() const { return CurrentFrame; }

	UFUNCTION(BlueprintPure, Category = "GIF")
	int32 GetNumFrames() const { return Source.IsValid() ? Source->GetNumFrames() : 0; }

	UFUNCTION(BlueprintPure, Category = "GIF")
	bool IsPlaying() const { return bPlaying; }

	UFUNCTION(BlueprintPure, Category = "GIF")
	FIntPoint GetDimensions() const;

	/** Playback rate multiplier (forward only in v1). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIF", meta = (ClampMin = "0.0"))
	float PlayRate = 1.0f;

	/**
	 * Master loop switch. When false the GIF plays exactly once and fires OnFinished,
	 * regardless of LoopCount. When true, LoopCount decides how many play-throughs.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIF")
	bool bLooping = true;

	/**
	 * Play-throughs before stopping when bLooping is true; 0 = forever. Seeded from
	 * the asset in Initialize; set it afterwards to override.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIF", meta = (ClampMin = "0", EditCondition = "bLooping"))
	int32 LoopCount = 0;

	/** Keep animating while the game is paused (HUD/menu/loading GIFs). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIF")
	bool bIgnorePause = false;

	/** Advance by real time, ignoring world time dilation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIF")
	bool bIgnoreTimeDilation = false;

	/** Fired each time playback wraps from the last frame back to the first. */
	UPROPERTY(BlueprintAssignable, Category = "GIF")
	FOnGifPlaybackEvent OnLooped;

	/** Fired once when a non-looping player reaches the final frame. */
	UPROPERTY(BlueprintAssignable, Category = "GIF")
	FOnGifPlaybackEvent OnFinished;

	/** Native, fired on the game thread whenever the displayed frame changes. */
	DECLARE_MULTICAST_DELEGATE(FOnFrameChangedNative);
	FOnFrameChangedNative OnFrameChangedNative;

	/** Driver entry point — called by the subsystem or the global ticker. */
	void TickPlayer(float DriverDeltaSeconds);

	UWorld* GetPlayerWorld() const { return World.Get(); }

	//~ UObject
	virtual void BeginDestroy() override;

private:
	FAnimatedImageDataPtr Source;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2DDynamic> OutputTexture;

	/** When set, this render target is the output instead of OutputTexture. */
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	TWeakObjectPtr<UWorld> World;

	/** Effective (clamped) per-frame delays and their running end-times. */
	TArray<float> CumulativeEndTimes;
	float TotalDuration = 0.0f;

	float AccumulatedTime = 0.0f;
	int32 CurrentFrame = INDEX_NONE;
	int32 LoopsCompleted = 0;
	bool bPlaying = false;
	bool bFinished = false; // reached the loop limit; Play() restarts rather than resumes
	bool bRegistered = false;
	bool bUsesGlobalTicker = false;

	void EnsureTexture();
	void ConfigureRenderTarget();
	FTextureResource* GetActiveResource() const;
	void RebuildTiming();
	int32 FrameAtTime(float WrappedTime) const;

	void SetCurrentFrame(int32 NewFrame, bool bForceUpload);
	void UploadFrameToGPU(int32 FrameIndex);

	void RegisterWithDriver();
	void UnregisterFromDriver();
};
