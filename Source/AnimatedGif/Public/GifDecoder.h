// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Containers/UnrealString.h"
#include "CoreTypes.h"
#include "IAnimatedImageDecoder.h"

/**
 * GIF decoder backed by the vendored stb_image. stb composites disposal methods,
 * transparency, partial frames and interlacing internally and returns full-canvas
 * RGBA frames + per-frame delays; we swizzle to BGRA and parse the NETSCAPE2.0
 * loop-count extension (which stb ignores).
 */
class ANIMATEDGIF_API FGifDecoder : public IAnimatedImageDecoder
{
public:
	virtual FString GetName() const override { return TEXT("GIF"); }
	virtual bool CanDecode(TConstArrayView<uint8> EncodedBytes) const override;
	virtual FAnimatedImageDataPtr Decode(TConstArrayView<uint8> EncodedBytes, FString& OutError) const override;

	/** Scan raw GIF bytes for the NETSCAPE2.0 loop count. Returns 0 (infinite) if absent. */
	static int32 ParseLoopCount(TConstArrayView<uint8> EncodedBytes);
};
