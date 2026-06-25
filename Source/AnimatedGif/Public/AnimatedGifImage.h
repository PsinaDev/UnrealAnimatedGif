// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Internationalization/Text.h"
#include "Math/Color.h"
#include "Templates/SharedPointer.h"
#include "UObject/ObjectMacros.h"
#include "Components/Widget.h"
#include "AnimatedGifImage.generated.h"

class UGifAsset;
class UGifPlayer;
class SAnimatedGifImage;

/**
 * UMG widget that plays a UGifAsset. Owns a UGifPlayer (the clock + GPU texture)
 * and repaints the Slate widget only when the player reports a new frame.
 */
UCLASS()
class ANIMATEDGIF_API UAnimatedGifImage : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animated GIF")
	TObjectPtr<UGifAsset> GifAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animated GIF")
	FLinearColor ColorAndOpacity = FLinearColor::White;

	/** Start playing as soon as the widget is constructed (runtime only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animated GIF")
	bool bAutoPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animated GIF")
	bool bLooping = true;

	/** -1 = use the asset's loop count; >= 0 overrides it (0 = loop forever). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animated GIF", meta = (ClampMin = "-1", EditCondition = "bLooping"))
	int32 LoopCountOverride = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animated GIF", meta = (ClampMin = "0.0"))
	float PlayRate = 1.0f;

#if WITH_EDITORONLY_DATA
	/** Play the GIF inside the UMG designer preview (off by default to keep the editor light). */
	UPROPERTY(EditAnywhere, Category = "Animated GIF", meta = (DisplayName = "Preview In Designer"))
	bool bPreviewInDesigner = false;
#endif

	UFUNCTION(BlueprintCallable, Category = "Animated GIF")
	void SetGifAsset(UGifAsset* InAsset);

	UFUNCTION(BlueprintCallable, Category = "Animated GIF")
	void Play();

	UFUNCTION(BlueprintCallable, Category = "Animated GIF")
	void Pause();

	UFUNCTION(BlueprintCallable, Category = "Animated GIF")
	void Stop();

	UFUNCTION(BlueprintCallable, Category = "Animated GIF")
	void SetColorAndOpacity(FLinearColor InColor);

	/** The underlying player (texture, playback state, events). */
	UFUNCTION(BlueprintPure, Category = "Animated GIF")
	UGifPlayer* GetPlayer() const { return Player; }

	//~ UWidget
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGifPlayer> Player;

	UPROPERTY(Transient)
	TObjectPtr<UGifAsset> BuiltAsset;

	TSharedPtr<SAnimatedGifImage> MyGif;

	void RebuildPlayer();
	void HandleFrameChanged();
};
