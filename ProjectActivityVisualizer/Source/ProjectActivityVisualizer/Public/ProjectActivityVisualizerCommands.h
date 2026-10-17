// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "ProjectActivityVisualizerStyle.h"

class FProjectActivityVisualizerCommands : public TCommands<FProjectActivityVisualizerCommands>
{
public:

	FProjectActivityVisualizerCommands()
		: TCommands<FProjectActivityVisualizerCommands>(TEXT("ProjectActivityVisualizer"), NSLOCTEXT("Contexts", "ProjectActivityVisualizer", "ProjectActivityVisualizer Plugin"), NAME_None, FProjectActivityVisualizerStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};