// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectActivityVisualizerCommands.h"

#define LOCTEXT_NAMESPACE "FProjectActivityVisualizerModule"

void FProjectActivityVisualizerCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "ProjectActivityVisualizer", "Bring up ProjectActivityVisualizer window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
