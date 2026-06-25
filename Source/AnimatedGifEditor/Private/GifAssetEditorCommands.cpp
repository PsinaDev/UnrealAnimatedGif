// Copyright PsinaDev. All Rights Reserved.

#include "GifAssetEditorCommands.h"

#define LOCTEXT_NAMESPACE "GifAssetEditorCommands"

void FGifAssetEditorCommands::RegisterCommands()
{
	UI_COMMAND(Play, "Play", "Play the GIF animation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Pause, "Pause", "Pause the GIF animation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Stop, "Stop", "Stop and rewind to the first frame", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ToggleLoop, "Loop", "Toggle looping playback", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE
