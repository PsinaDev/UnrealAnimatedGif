// Copyright PsinaDev. All Rights Reserved.

#include "AnimatedGifFunctionLibrary.h"
#include "AnimatedGifModule.h"
#include "IAnimatedImageDecoder.h"
#include "GifAsset.h"
#include "GifPlayer.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimatedGifFunctionLibrary)

UGifPlayer* UAnimatedGifFunctionLibrary::CreatePlayerFromSource(UObject* WorldContextObject, const FAnimatedImageDataRef& Source, bool bAutoPlay, bool bLooping, UTextureRenderTarget2D* RenderTarget)
{
	if (!Source->IsValid())
	{
		return nullptr;
	}

	UObject* Outer = WorldContextObject ? WorldContextObject : static_cast<UObject*>(GetTransientPackage());
	UGifPlayer* Player = NewObject<UGifPlayer>(Outer);
	Player->bLooping = bLooping;
	if (RenderTarget)
	{
		Player->SetRenderTarget(RenderTarget);
	}
	Player->Initialize(WorldContextObject, Source);

	if (bAutoPlay)
	{
		Player->Play();
	}
	return Player;
}

UGifPlayer* UAnimatedGifFunctionLibrary::CreateGifPlayer(UObject* WorldContextObject, UGifAsset* GifAsset, bool bAutoPlay, bool bLooping)
{
	if (!GifAsset)
	{
		return nullptr;
	}
	return CreatePlayerFromSource(WorldContextObject, GifAsset->GetFrameSource(), bAutoPlay, bLooping, GifAsset->LinkedRenderTarget);
}

UGifPlayer* UAnimatedGifFunctionLibrary::CreateGifPlayerFromBytes(UObject* WorldContextObject, const TArray<uint8>& GifBytes, bool bAutoPlay, bool bLooping)
{
	FString Error;
	FAnimatedImageDataPtr Source = FAnimatedImageDecoderRegistry::Get().Decode(GifBytes, Error);
	if (!Source.IsValid())
	{
		UE_LOG(LogAnimatedGif, Warning, TEXT("CreateGifPlayerFromBytes failed: %s"), *Error);
		return nullptr;
	}
	return CreatePlayerFromSource(WorldContextObject, Source.ToSharedRef(), bAutoPlay, bLooping);
}

UGifPlayer* UAnimatedGifFunctionLibrary::LoadGifPlayerFromFile(UObject* WorldContextObject, const FString& FilePath, bool bAutoPlay, bool bLooping)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FilePath))
	{
		UE_LOG(LogAnimatedGif, Warning, TEXT("LoadGifPlayerFromFile failed to read '%s'"), *FilePath);
		return nullptr;
	}
	return CreateGifPlayerFromBytes(WorldContextObject, Bytes, bAutoPlay, bLooping);
}
