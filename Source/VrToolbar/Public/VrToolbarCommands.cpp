// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VrToolbarPrivatePCH.h"
#include "VrToolbarCommands.h"

#define LOCTEXT_NAMESPACE "FVrToolbarModule"

void FVrToolbarCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Cardboard", "Connect/disconnect tethered Android device", EUserInterfaceActionType::ToggleButton, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
