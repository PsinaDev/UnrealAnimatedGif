// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AnimatedGifSettings.generated.h"

/**
 * Project-wide GIF playback defaults. Frame delays come from the file, but many
 * GIFs ship 0ms delays expecting browser behaviour (~10fps); these settings
 * reproduce that clamp.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Animated GIF"))
class ANIMATEDGIF_API UAnimatedGifSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Project"); }

	/** Frames whose stored delay is below this are bumped to FallbackFrameDelaySeconds. */
	UPROPERTY(EditAnywhere, config, Category = "Playback", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float MinFrameDelaySeconds = 0.02f;

	/** Delay used in place of a too-short frame delay. */
	UPROPERTY(EditAnywhere, config, Category = "Playback", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float FallbackFrameDelaySeconds = 0.1f;

	static const UAnimatedGifSettings& Get() { return *GetDefault<UAnimatedGifSettings>(); }
};
