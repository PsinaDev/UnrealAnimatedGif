// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Ticker.h"
#include "GifPlaybackSubsystem.generated.h"

class UGifPlayer;

/**
 * Drives all GIF players that belong to a world. Ticks with world delta time, so
 * playback is pause- and time-dilation-aware by default; per-player overrides let
 * HUD/menu GIFs keep running while paused.
 */
UCLASS()
class ANIMATEDGIF_API UGifPlaybackSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterPlayer(UGifPlayer* Player);
	void UnregisterPlayer(UGifPlayer* Player);

	//~ UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return ActivePlayers.Num() > 0; }
	virtual bool IsTickableWhenPaused() const override { return true; } // per-player pause handled in UGifPlayer::TickPlayer
	virtual void Deinitialize() override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGifPlayer>> ActivePlayers;
};

/**
 * Real-time fallback driver for players created without a world (e.g. loading
 * screens). Uses the core ticker; only alive while at least one such player exists.
 */
class FGifGlobalTicker
{
public:
	static FGifGlobalTicker& Get();

	void Register(UGifPlayer* Player);
	void Unregister(UGifPlayer* Player);
	void Shutdown();

private:
	bool Tick(float DeltaSeconds);

	TArray<TWeakObjectPtr<UGifPlayer>> Players;
	FTSTicker::FDelegateHandle TickerHandle;
};
