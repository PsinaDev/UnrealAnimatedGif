// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

class FGifAssetEditorCommands : public TCommands<FGifAssetEditorCommands>
{
public:
	FGifAssetEditorCommands()
		: TCommands<FGifAssetEditorCommands>(
			TEXT("GifAssetEditor"),
			NSLOCTEXT("Contexts", "GifAssetEditor", "GIF Asset Editor"),
			NAME_None,
			FAppStyle::GetAppStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> Play;
	TSharedPtr<FUICommandInfo> Pause;
	TSharedPtr<FUICommandInfo> Stop;
	TSharedPtr<FUICommandInfo> ToggleLoop;
};
