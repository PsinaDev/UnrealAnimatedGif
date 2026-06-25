// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimatedImageTypes.h"
#include "AnimatedGifFunctionLibrary.generated.h"

class UGifAsset;
class UGifPlayer;
class UTextureRenderTarget2D;

/** Helpers to spin up GIF players from assets, bytes or files. */
UCLASS()
class ANIMATEDGIF_API UAnimatedGifFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Create a player from an imported asset. */
	UFUNCTION(BlueprintCallable, Category = "Animated GIF", meta = (WorldContext = "WorldContextObject"))
	static UGifPlayer* CreateGifPlayer(UObject* WorldContextObject, UGifAsset* GifAsset, bool bAutoPlay = true, bool bLooping = true);

	/**
	 * Decode raw GIF bytes and create a player. Synchronous — decode happens on the
	 * calling thread; prefer LoadAnimatedGifFromBytes (async node) for large GIFs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animated GIF", meta = (WorldContext = "WorldContextObject"))
	static UGifPlayer* CreateGifPlayerFromBytes(UObject* WorldContextObject, const TArray<uint8>& GifBytes, bool bAutoPlay = true, bool bLooping = true);

	/** Read a .gif from disk and create a player. Synchronous; prefer the async node. */
	UFUNCTION(BlueprintCallable, Category = "Animated GIF", meta = (WorldContext = "WorldContextObject"))
	static UGifPlayer* LoadGifPlayerFromFile(UObject* WorldContextObject, const FString& FilePath, bool bAutoPlay = true, bool bLooping = true);

	/** Shared C++ entry: build a player from an already-decoded source, optionally driving a render target. */
	static UGifPlayer* CreatePlayerFromSource(UObject* WorldContextObject, const FAnimatedImageDataRef& Source, bool bAutoPlay, bool bLooping, UTextureRenderTarget2D* RenderTarget = nullptr);
};
