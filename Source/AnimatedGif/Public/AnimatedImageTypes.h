// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Immutable, fully-decoded animation shared between consumers.
 *
 * Pixels hold every frame already composited to the full canvas (disposal
 * methods, transparency and partial-rect frames are resolved at decode time),
 * so playback is a pure index->memcpy with no per-frame compositing. Stored as
 * BGRA8 top-down to match PF_B8G8R8A8 and feed RHIUpdateTexture2D directly.
 *
 * Not a UObject: it is ref-counted and read-only after decode, so the render
 * thread can sample one player's frame while the game thread advances another.
 */
struct FAnimatedImageData
{
	int32 Width = 0;
	int32 Height = 0;

	/** GIF NETSCAPE2.0 loop count; 0 means loop forever. */
	int32 LoopCount = 0;

	/** Display duration of each frame, in seconds. Size == frame count. */
	TArray<float> FrameDelays;

	/** BGRA8, top-down, NumFrames * Width * Height * 4 bytes back-to-back. */
	TArray64<uint8> Pixels;

	int32 GetNumFrames() const { return FrameDelays.Num(); }
	int64 GetFrameSizeBytes() const { return static_cast<int64>(Width) * Height * 4; }

	const uint8* GetFrameData(int32 FrameIndex) const
	{
		return Pixels.GetData() + static_cast<int64>(FrameIndex) * GetFrameSizeBytes();
	}

	bool IsValid() const
	{
		return Width > 0 && Height > 0 && GetNumFrames() > 0
			&& Pixels.Num() == static_cast<int64>(GetNumFrames()) * GetFrameSizeBytes();
	}

	SIZE_T GetAllocatedSize() const
	{
		return Pixels.GetAllocatedSize() + FrameDelays.GetAllocatedSize();
	}
};

using FAnimatedImageDataRef = TSharedRef<FAnimatedImageData, ESPMode::ThreadSafe>;
using FAnimatedImageDataPtr = TSharedPtr<FAnimatedImageData, ESPMode::ThreadSafe>;
