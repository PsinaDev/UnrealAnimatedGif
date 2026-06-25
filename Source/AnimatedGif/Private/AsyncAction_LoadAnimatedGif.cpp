// Copyright PsinaDev. All Rights Reserved.

#include "AsyncAction_LoadAnimatedGif.h"
#include "AnimatedGifFunctionLibrary.h"
#include "AnimatedGifModule.h"
#include "IAnimatedImageDecoder.h"
#include "GifPlayer.h"

#include "Async/Async.h"
#include "Misc/FileHelper.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_LoadAnimatedGif)

UAsyncAction_LoadAnimatedGif* UAsyncAction_LoadAnimatedGif::LoadAnimatedGifFromFile(UObject* WorldContextObject, const FString& FilePath, bool bAutoPlay, bool bLooping)
{
	UAsyncAction_LoadAnimatedGif* Action = NewObject<UAsyncAction_LoadAnimatedGif>();
	Action->WorldContextObject = WorldContextObject;
	Action->FilePath = FilePath;
	Action->bFromFile = true;
	Action->bAutoPlay = bAutoPlay;
	Action->bLooping = bLooping;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UAsyncAction_LoadAnimatedGif* UAsyncAction_LoadAnimatedGif::LoadAnimatedGifFromBytes(UObject* WorldContextObject, const TArray<uint8>& GifBytes, bool bAutoPlay, bool bLooping)
{
	UAsyncAction_LoadAnimatedGif* Action = NewObject<UAsyncAction_LoadAnimatedGif>();
	Action->WorldContextObject = WorldContextObject;
	Action->Bytes = GifBytes;
	Action->bFromFile = false;
	Action->bAutoPlay = bAutoPlay;
	Action->bLooping = bLooping;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UAsyncAction_LoadAnimatedGif::Activate()
{
	TWeakObjectPtr<UAsyncAction_LoadAnimatedGif> WeakThis(this);
	const bool bFile = bFromFile;
	FString Path = FilePath;
	TArray<uint8> LocalBytes = MoveTemp(Bytes);

	Async(EAsyncExecution::ThreadPool,
		[WeakThis, bFile, Path = MoveTemp(Path), LocalBytes = MoveTemp(LocalBytes)]() mutable
		{
			FString Error;
			TArray<uint8> Data;
			if (bFile)
			{
				if (!FFileHelper::LoadFileToArray(Data, *Path))
				{
					Error = FString::Printf(TEXT("Failed to read file: %s"), *Path);
				}
			}
			else
			{
				Data = MoveTemp(LocalBytes);
			}

			FAnimatedImageDataPtr Source;
			if (Error.IsEmpty())
			{
				Source = FAnimatedImageDecoderRegistry::Get().Decode(Data, Error);
			}

			// UObject creation must happen on the game thread.
			AsyncTask(ENamedThreads::GameThread, [WeakThis, Source, Error = MoveTemp(Error)]()
			{
				UAsyncAction_LoadAnimatedGif* StrongThis = WeakThis.Get();
				if (!StrongThis)
				{
					return;
				}

				if (Source.IsValid() && Source->IsValid())
				{
					UGifPlayer* Player = UAnimatedGifFunctionLibrary::CreatePlayerFromSource(
						StrongThis->WorldContextObject.Get(), Source.ToSharedRef(), StrongThis->bAutoPlay, StrongThis->bLooping);
					StrongThis->PendingPlayer = Player; // root across the broadcast
					StrongThis->OnLoaded.Broadcast(Player);
					StrongThis->PendingPlayer = nullptr;
				}
				else
				{
					StrongThis->OnFailed.Broadcast(Error.IsEmpty() ? TEXT("GIF decode failed.") : Error);
				}

				StrongThis->SetReadyToDestroy();
			});
		});
}
