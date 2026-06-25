// Copyright PsinaDev. All Rights Reserved.

#include "GifPlaybackSubsystem.h"
#include "GifPlayer.h"

//----------------------------------------------------------------------------
// UGifPlaybackSubsystem
//----------------------------------------------------------------------------

void UGifPlaybackSubsystem::RegisterPlayer(UGifPlayer* Player)
{
	if (Player)
	{
		ActivePlayers.AddUnique(Player);
	}
}

void UGifPlaybackSubsystem::UnregisterPlayer(UGifPlayer* Player)
{
	ActivePlayers.RemoveSingleSwap(Player, EAllowShrinking::No);
}

void UGifPlaybackSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Players may unregister themselves mid-tick (on finish), so iterate a snapshot.
	TArray<TObjectPtr<UGifPlayer>, TInlineAllocator<16>> Snapshot(ActivePlayers);
	for (UGifPlayer* Player : Snapshot)
	{
		if (Player)
		{
			Player->TickPlayer(DeltaTime);
		}
	}
}

TStatId UGifPlaybackSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGifPlaybackSubsystem, STATGROUP_Tickables);
}

void UGifPlaybackSubsystem::Deinitialize()
{
	ActivePlayers.Reset();
	Super::Deinitialize();
}

//----------------------------------------------------------------------------
// FGifGlobalTicker
//----------------------------------------------------------------------------

FGifGlobalTicker& FGifGlobalTicker::Get()
{
	static FGifGlobalTicker Instance;
	return Instance;
}

void FGifGlobalTicker::Register(UGifPlayer* Player)
{
	if (!Player)
	{
		return;
	}

	Players.AddUnique(Player);

	if (!TickerHandle.IsValid())
	{
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FGifGlobalTicker::Tick));
	}
}

void FGifGlobalTicker::Unregister(UGifPlayer* Player)
{
	Players.RemoveSingleSwap(Player, EAllowShrinking::No);
}

bool FGifGlobalTicker::Tick(float DeltaSeconds)
{
	for (int32 i = Players.Num() - 1; i >= 0; --i)
	{
		if (UGifPlayer* Player = Players[i].Get())
		{
			Player->TickPlayer(DeltaSeconds);
		}
		else
		{
			Players.RemoveAtSwap(i, EAllowShrinking::No);
		}
	}

	if (Players.IsEmpty())
	{
		// Nothing left to drive: drop the ticker; Register() re-adds it on demand.
		TickerHandle.Reset();
		return false;
	}
	return true;
}

void FGifGlobalTicker::Shutdown()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	Players.Reset();
}
