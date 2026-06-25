// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimatedImageTypes.h"

/**
 * Decoder seam: one implementation per animated-image format. v1 ships GIF only,
 * but APNG/WebP slot in by registering another decoder — no consumer changes.
 * Implementations must be thread-safe (decode runs on worker threads).
 */
class IAnimatedImageDecoder
{
public:
	virtual ~IAnimatedImageDecoder() = default;

	/** Human-readable name for logging (e.g. "GIF"). */
	virtual FString GetName() const = 0;

	/** Cheap header sniff; must not allocate or fully parse. */
	virtual bool CanDecode(TConstArrayView<uint8> EncodedBytes) const = 0;

	/** Decode all frames. Returns null and fills OutError on failure. */
	virtual FAnimatedImageDataPtr Decode(TConstArrayView<uint8> EncodedBytes, FString& OutError) const = 0;
};

/**
 * Process-wide registry of decoders. Decoders are registered at module startup;
 * Decode() sniffs the bytes and dispatches to the first decoder that accepts them.
 */
class ANIMATEDGIF_API FAnimatedImageDecoderRegistry
{
public:
	static FAnimatedImageDecoderRegistry& Get();

	void RegisterDecoder(const TSharedRef<IAnimatedImageDecoder>& Decoder);
	void Reset();

	/** Pick a decoder by sniffing the header and decode. Null + OutError on failure. */
	FAnimatedImageDataPtr Decode(TConstArrayView<uint8> EncodedBytes, FString& OutError) const;

private:
	TArray<TSharedRef<IAnimatedImageDecoder>> Decoders;
};
