// Copyright PsinaDev. All Rights Reserved.

#include "GifAsset.h"
#include "AnimatedGifModule.h"
#include "Misc/ScopeLock.h"

#if WITH_EDITORONLY_DATA
#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#endif

float UGifAsset::GetDurationSeconds() const
{
	float Total = 0.0f;
	for (float Delay : FrameDelays)
	{
		Total += Delay;
	}
	return Total;
}

void UGifAsset::SetFromDecoded(const FAnimatedImageData& Decoded)
{
	if (!Decoded.IsValid())
	{
		UE_LOG(LogAnimatedGif, Warning, TEXT("SetFromDecoded called with an invalid image; asset '%s' left empty."), *GetName());
		return;
	}

	Width = Decoded.Width;
	Height = Decoded.Height;
	LoopCount = Decoded.LoopCount;
	FrameDelays = Decoded.FrameDelays;

	FrameData.Lock(LOCK_READ_WRITE);
	void* Dest = FrameData.Realloc(Decoded.Pixels.Num());
	FMemory::Memcpy(Dest, Decoded.Pixels.GetData(), Decoded.Pixels.Num());
	FrameData.Unlock();
	// Keep the pixel payload inside the package so the cooked load never depends on a separate .ubulk.
	FrameData.SetBulkDataFlags(BULKDATA_ForceInlinePayload);

	FScopeLock Lock(&SourceLock);
	CachedSource.Reset();
}

FAnimatedImageDataRef UGifAsset::GetFrameSource()
{
	FScopeLock Lock(&SourceLock);

	if (CachedSource.IsValid())
	{
		return CachedSource.ToSharedRef();
	}

	FAnimatedImageDataRef Source = MakeShared<FAnimatedImageData, ESPMode::ThreadSafe>();
	Source->Width = Width;
	Source->Height = Height;
	Source->LoopCount = LoopCount;
	Source->FrameDelays = FrameDelays;

	const int64 Size = FrameData.GetBulkDataSize();
	if (Size > 0)
	{
		Source->Pixels.SetNumUninitialized(Size);
		const void* Data = FrameData.LockReadOnly();
		FMemory::Memcpy(Source->Pixels.GetData(), Data, Size);
		FrameData.Unlock();
	}

	if (!Source->IsValid())
	{
		UE_LOG(LogAnimatedGif, Warning, TEXT("GIF asset '%s' produced an invalid frame source (%dx%d, %d frames, %lld bytes)."),
			*GetName(), Width, Height, GetNumFrames(), (long long)Size);
	}

	CachedSource = Source;
	return Source;
}

#if WITH_EDITORONLY_DATA
void UGifAsset::SetRawGifBytes(TConstArrayView<uint8> Bytes)
{
	RawGifData.Lock(LOCK_READ_WRITE);
	void* Dest = RawGifData.Realloc(Bytes.Num());
	FMemory::Memcpy(Dest, Bytes.GetData(), Bytes.Num());
	RawGifData.Unlock();
}

void UGifAsset::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
	Super::PostInitProperties();
}

UTexture2D* UGifAsset::GetThumbnailTexture()
{
	if (ThumbnailTexture)
	{
		return ThumbnailTexture;
	}

	const FAnimatedImageDataRef Src = GetFrameSource();
	if (!Src->IsValid())
	{
		return nullptr;
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(Src->Width, Src->Height, PF_B8G8R8A8);
	if (!Texture)
	{
		return nullptr;
	}
	Texture->SRGB = true;

	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* Dest = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Dest, Src->GetFrameData(0), Src->GetFrameSizeBytes());
	Mip.BulkData.Unlock();
	Texture->UpdateResource();

	ThumbnailTexture = Texture;
	return Texture;
}
#endif

void UGifAsset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// Runtime payload: always present.
	FrameData.Serialize(Ar, this);

#if WITH_EDITORONLY_DATA
	// Editor-only payload: skipped for cooked (editor-stripped) archives.
	if (!Ar.IsFilterEditorOnly())
	{
		RawGifData.Serialize(Ar, this);
	}
#endif
}
