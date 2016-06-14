// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateBasics.h"
#include "VrToolbarStyle.h"

class FVrToolbarCommands : public TCommands<FVrToolbarCommands>
{
public:

	FVrToolbarCommands()
		: TCommands<FVrToolbarCommands>(TEXT("VrToolbar"), NSLOCTEXT("Contexts", "VrToolbar", "VrToolbar Plugin"), NAME_None, FVrToolbarStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
