// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

ANIMATEDGIF_API DECLARE_LOG_CATEGORY_EXTERN(LogAnimatedGif, Log, All);

class FAnimatedGifModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
