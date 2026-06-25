// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "AnimatedGifFactory.generated.h"

/** Imports a .gif into a UGifAsset (decode + bake frames). */
UCLASS()
class UAnimatedGifFactory : public UFactory
{
	GENERATED_BODY()

public:
	// UFactory only declares a (const FObjectInitializer&) ctor, so forward to it.
	UAnimatedGifFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual UObject* FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
		UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;
};

/** Adds reimport support for UGifAsset. */
UCLASS()
class UReimportAnimatedGifFactory : public UAnimatedGifFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	//~ FReimportHandler
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override { return ImportPriority; }
};
