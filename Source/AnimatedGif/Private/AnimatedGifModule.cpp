// Copyright PsinaDev. All Rights Reserved.

#include "AnimatedGifModule.h"
#include "IAnimatedImageDecoder.h"
#include "GifDecoder.h"
#include "GifPlaybackSubsystem.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogAnimatedGif);

FAnimatedImageDecoderRegistry& FAnimatedImageDecoderRegistry::Get()
{
	static FAnimatedImageDecoderRegistry Instance;
	return Instance;
}

void FAnimatedImageDecoderRegistry::RegisterDecoder(const TSharedRef<IAnimatedImageDecoder>& Decoder)
{
	Decoders.Add(Decoder);
}

void FAnimatedImageDecoderRegistry::Reset()
{
	Decoders.Reset();
}

FAnimatedImageDataPtr FAnimatedImageDecoderRegistry::Decode(TConstArrayView<uint8> EncodedBytes, FString& OutError) const
{
	for (const TSharedRef<IAnimatedImageDecoder>& Decoder : Decoders)
	{
		if (Decoder->CanDecode(EncodedBytes))
		{
			return Decoder->Decode(EncodedBytes, OutError);
		}
	}
	OutError = TEXT("No registered decoder accepts this data.");
	return nullptr;
}

void FAnimatedGifModule::StartupModule()
{
	FAnimatedImageDecoderRegistry::Get().RegisterDecoder(MakeShared<FGifDecoder>());
}

void FAnimatedGifModule::ShutdownModule()
{
	FAnimatedImageDecoderRegistry::Get().Reset();
	FGifGlobalTicker::Get().Shutdown();
}

IMPLEMENT_MODULE(FAnimatedGifModule, AnimatedGif)
