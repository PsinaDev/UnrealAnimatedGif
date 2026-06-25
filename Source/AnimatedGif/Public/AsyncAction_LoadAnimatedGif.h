// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncAction_LoadAnimatedGif.generated.h"

class UGifPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimatedGifLoaded, UGifPlayer*, Player);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimatedGifLoadFailed, const FString&, Error);

/**
 * Async latent node: reads/decodes a GIF on a worker thread, then creates the
 * player on the game thread. Use this (not the synchronous library calls) for
 * anything large or loaded at runtime, so the game thread never blocks on decode.
 */
UCLASS()
class ANIMATEDGIF_API UAsyncAction_LoadAnimatedGif : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnAnimatedGifLoaded OnLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnAnimatedGifLoadFailed OnFailed;

	UFUNCTION(BlueprintCallable, Category = "Animated GIF",
		meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_LoadAnimatedGif* LoadAnimatedGifFromFile(UObject* WorldContextObject, const FString& FilePath, bool bAutoPlay = true, bool bLooping = true);

	UFUNCTION(BlueprintCallable, Category = "Animated GIF",
		meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_LoadAnimatedGif* LoadAnimatedGifFromBytes(UObject* WorldContextObject, const TArray<uint8>& GifBytes, bool bAutoPlay = true, bool bLooping = true);

	virtual void Activate() override;

private:
	UPROPERTY()
	TWeakObjectPtr<UObject> WorldContextObject;

	/** Roots the created player across the OnLoaded broadcast so GC can't reclaim it before the BP stores it. */
	UPROPERTY()
	TObjectPtr<UGifPlayer> PendingPlayer;

	FString FilePath;
	TArray<uint8> Bytes;
	bool bFromFile = false;
	bool bAutoPlay = true;
	bool bLooping = true;
};
