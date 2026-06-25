// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "CoreTypes.h"
#include "HAL/CriticalSection.h"
#include "Math/IntPoint.h"
#include "Serialization/Archive.h"
#include "UObject/ObjectMacros.h"
#include "Serialization/BulkData.h"
#include "AnimatedImageTypes.h"
#include "GifAsset.generated.h"

/**
 * Imported, cooked GIF. Stores lightweight metadata as reflected properties (so
 * the asset registry sees it without loading pixels) and the decoded BGRA frames
 * in manually-serialized bulk data. Immutable at runtime; one asset can back many
 * independent UGifPlayer instances.
 */
UCLASS(BlueprintType)
class ANIMATEDGIF_API UGifAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "GIF")
	int32 Width = 0;

	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "GIF")
	int32 Height = 0;

	/**
	 * Number of play-throughs before playback stops; 0 = loop forever. Seeded from
	 * the GIF's NETSCAPE2.0 extension on import, editable here, and used by UGifPlayer
	 * as the default loop count (a player/widget may still override it).
	 */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "GIF")
	int32 LoopCount = 0;

	/** Per-frame display duration in seconds (raw, before the playback clamp). */
	UPROPERTY(VisibleAnywhere, Category = "GIF")
	TArray<float> FrameDelays;

	/**
	 * Optional render target a player drives so the GIF can be sampled in a material
	 * graph. At runtime the player reconfigures it to this GIF's size and BGRA8 format
	 * and uses it as its output texture. Shared: one logical playback per target — if
	 * several players run this asset they all write here (last write wins).
	 */
	UPROPERTY(EditAnywhere, Category = "GIF|Material")
	TObjectPtr<class UTextureRenderTarget2D> LinkedRenderTarget;

	int32 GetNumFrames() const { return FrameDelays.Num(); }

	UFUNCTION(BlueprintPure, Category = "GIF")
	float GetDurationSeconds() const;

	UFUNCTION(BlueprintPure, Category = "GIF")
	FIntPoint GetDimensions() const { return FIntPoint(Width, Height); }

	/** Replace the payload from a freshly decoded image (import / reimport). */
	void SetFromDecoded(const FAnimatedImageData& Decoded);

	/**
	 * Shared, immutable frame source for playback. Built once from bulk data on
	 * first use and kept resident for the asset's lifetime.
	 */
	FAnimatedImageDataRef GetFrameSource();

#if WITH_EDITORONLY_DATA
	/** Store the original .gif bytes (for reimport when the source file is gone). */
	void SetRawGifBytes(TConstArrayView<uint8> Bytes);

	/** Lazily-built frame-0 texture used for the content-browser thumbnail. */
	class UTexture2D* GetThumbnailTexture();

	UPROPERTY(VisibleAnywhere, Instanced, Category = "ImportSettings")
	TObjectPtr<class UAssetImportData> AssetImportData;
#endif

	//~ UObject
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITORONLY_DATA
	virtual void PostInitProperties() override;
#endif

private:
	/** BGRA8, top-down, NumFrames * Width * Height * 4. Runtime payload. */
	FByteBulkData FrameData;

#if WITH_EDITORONLY_DATA
	/** Original encoded bytes; editor-only, stripped on cook. */
	FByteBulkData RawGifData;

	UPROPERTY(Transient)
	TObjectPtr<class UTexture2D> ThumbnailTexture;
#endif

	/** Lazily materialized shared source; weakly cached so it can be released. */
	FAnimatedImageDataPtr CachedSource;

	/** Guards CachedSource build/reset against concurrent GetFrameSource callers. */
	mutable FCriticalSection SourceLock;
};
