// Copyright PsinaDev. All Rights Reserved.

#include "GifDecoder.h"
#include "AnimatedGifModule.h"
#include "Misc/ScopeExit.h"

// Pull in stb declarations only (the implementation lives in StbImageImpl.cpp).
THIRD_PARTY_INCLUDES_START
#include "stb_image.h"
THIRD_PARTY_INCLUDES_END

bool FGifDecoder::CanDecode(TConstArrayView<uint8> Bytes) const
{
	// "GIF87a" / "GIF89a" magic.
	return Bytes.Num() >= 6
		&& Bytes[0] == 'G' && Bytes[1] == 'I' && Bytes[2] == 'F' && Bytes[3] == '8'
		&& (Bytes[4] == '7' || Bytes[4] == '9') && Bytes[5] == 'a';
}

int32 FGifDecoder::ParseLoopCount(TConstArrayView<uint8> Bytes)
{
	// Application Extension: 0x21 0xFF 0x0B "NETSCAPE2.0" <0x03 0x01 loop16le 0x00>.
	static const uint8 Tag[11] = { 'N','E','T','S','C','A','P','E','2','.','0' };
	const uint8* P = Bytes.GetData();
	const int32 N = Bytes.Num();
	for (int32 i = 0; i + 17 < N; ++i)
	{
		if (P[i] == 0x21 && P[i + 1] == 0xFF && P[i + 2] == 0x0B
			&& FMemory::Memcmp(P + i + 3, Tag, sizeof(Tag)) == 0)
		{
			const int32 j = i + 14; // start of the sub-block following the tag
			if (P[j] == 0x03 && P[j + 1] == 0x01)
			{
				return static_cast<int32>(P[j + 2]) | (static_cast<int32>(P[j + 3]) << 8);
			}
		}
	}
	return 0; // not present -> loop forever
}

FAnimatedImageDataPtr FGifDecoder::Decode(TConstArrayView<uint8> Bytes, FString& OutError) const
{
	if (!CanDecode(Bytes))
	{
		OutError = TEXT("Buffer is not a GIF (bad magic).");
		return nullptr;
	}

	int Width = 0, Height = 0, FrameCount = 0, Channels = 0;
	int* DelaysMs = nullptr; // stb allocates; one entry per frame, in milliseconds

	stbi_uc* Decoded = stbi_load_gif_from_memory(
		Bytes.GetData(), Bytes.Num(), &DelaysMs, &Width, &Height, &FrameCount, &Channels, /*req_comp=*/4);

	if (Decoded == nullptr)
	{
		OutError = FString::Printf(TEXT("stb_image failed: %hs"), stbi_failure_reason());
		return nullptr;
	}
	ON_SCOPE_EXIT
	{
		stbi_image_free(Decoded);
		stbi_image_free(DelaysMs); // same allocator; safe on null
	};

	if (Width <= 0 || Height <= 0 || FrameCount <= 0)
	{
		OutError = TEXT("GIF decoded to an empty image.");
		return nullptr;
	}

	const int64 FrameBytes = static_cast<int64>(Width) * Height * 4;
	const int64 PixelCount = static_cast<int64>(Width) * Height;

	FAnimatedImageDataRef Data = MakeShared<FAnimatedImageData, ESPMode::ThreadSafe>();
	Data->Width = Width;
	Data->Height = Height;
	Data->LoopCount = ParseLoopCount(Bytes);
	Data->FrameDelays.Reserve(FrameCount);
	Data->Pixels.SetNumUninitialized(static_cast<int64>(FrameCount) * FrameBytes);

	for (int32 Frame = 0; Frame < FrameCount; ++Frame)
	{
		Data->FrameDelays.Add((DelaysMs ? DelaysMs[Frame] : 0) / 1000.0f);

		// stb emits RGBA; PF_B8G8R8A8 wants BGRA. Swizzle once, here, off the render thread.
		const uint8* Src = Decoded + static_cast<int64>(Frame) * FrameBytes;
		uint8* Dst = Data->Pixels.GetData() + static_cast<int64>(Frame) * FrameBytes;
		for (int64 Px = 0; Px < PixelCount; ++Px)
		{
			Dst[0] = Src[2];
			Dst[1] = Src[1];
			Dst[2] = Src[0];
			Dst[3] = Src[3];
			Src += 4;
			Dst += 4;
		}
	}

	UE_LOG(LogAnimatedGif, Verbose, TEXT("Decoded GIF: %dx%d, %d frames, loop=%d"),
		Width, Height, FrameCount, Data->LoopCount);

	return Data;
}
