// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VrToolbarPrivatePCH.h"
#include "VrToolbarCommands.h"

#define LOCTEXT_NAMESPACE "FVrToolbarModule"

void FVrToolbarCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Cardboard", "Connect/disconnect tethered Android device", EUserInterfaceActionType::ToggleButton, FInputGesture());
  UI_COMMAND(InstallAction, "Install Drivers", "Install drivers for an Android device", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
