// Copyright PsinaDev. All Rights Reserved.

#include "AnimatedGifFactory.h"
#include "GifAsset.h"
#include "IAnimatedImageDecoder.h"
#include "AnimatedGifModule.h"

#include "Editor.h"
#include "Subsystems/ImportSubsystem.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/Paths.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimatedGifFactory)

UAnimatedGifFactory::UAnimatedGifFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UGifAsset::StaticClass();
	Formats.Add(TEXT("gif;Animated GIF"));
	bEditorImport = true;
	bCreateNew = false;
	bText = false;
}

bool UAnimatedGifFactory::FactoryCanImport(const FString& Filename)
{
	return FPaths::GetExtension(Filename).Equals(TEXT("gif"), ESearchCase::IgnoreCase);
}

UObject* UAnimatedGifFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

	const int64 NumBytes = static_cast<int64>(BufferEnd - Buffer);
	const TConstArrayView<uint8> Bytes(Buffer, NumBytes);

	FString Error;
	const FAnimatedImageDataPtr Source = FAnimatedImageDecoderRegistry::Get().Decode(Bytes, Error);

	UGifAsset* Asset = nullptr;
	if (Source.IsValid() && Source->IsValid())
	{
		Asset = NewObject<UGifAsset>(InParent, InClass, InName, Flags);
		Asset->SetFromDecoded(*Source);
#if WITH_EDITORONLY_DATA
		Asset->SetRawGifBytes(Bytes);
		if (Asset->AssetImportData)
		{
			Asset->AssetImportData->Update(GetCurrentFilename());
		}
#endif
		UE_LOG(LogAnimatedGif, Log, TEXT("Imported GIF '%s' (%dx%d, %d frames)."),
			*InName.ToString(), Asset->Width, Asset->Height, Asset->GetNumFrames());
	}
	else
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Animated GIF import failed for '%s': %s"), *InName.ToString(), *Error);
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Asset);
	return Asset;
}

//----------------------------------------------------------------------------
// Reimport
//----------------------------------------------------------------------------

bool UReimportAnimatedGifFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
#if WITH_EDITORONLY_DATA
	if (UGifAsset* Asset = Cast<UGifAsset>(Obj))
	{
		if (Asset->AssetImportData)
		{
			Asset->AssetImportData->ExtractFilenames(OutFilenames);
			return true;
		}
	}
#endif
	return false;
}

void UReimportAnimatedGifFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
#if WITH_EDITORONLY_DATA
	if (UGifAsset* Asset = Cast<UGifAsset>(Obj))
	{
		if (Asset->AssetImportData && NewReimportPaths.Num() > 0)
		{
			Asset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
		}
	}
#endif
}

EReimportResult::Type UReimportAnimatedGifFactory::Reimport(UObject* Obj)
{
	UGifAsset* Asset = Cast<UGifAsset>(Obj);
	if (!Asset)
	{
		return EReimportResult::Failed;
	}

#if WITH_EDITORONLY_DATA
	const FString Filename = Asset->AssetImportData ? Asset->AssetImportData->GetFirstFilename() : FString();
#else
	const FString Filename;
#endif
	if (Filename.IsEmpty() || !FPaths::FileExists(Filename))
	{
		return EReimportResult::Failed;
	}

	bool bCanceled = false;
	UObject* Result = UFactory::StaticImportObject(
		Asset->GetClass(), Asset->GetOuter(), Asset->GetFName(), RF_Public | RF_Standalone,
		bCanceled, *Filename, nullptr, this);

	if (Result)
	{
		return EReimportResult::Succeeded;
	}
	return bCanceled ? EReimportResult::Cancelled : EReimportResult::Failed;
}
