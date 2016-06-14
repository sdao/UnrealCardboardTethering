// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VrToolbarPrivatePCH.h"

#include "SlateBasics.h"
#include "SlateExtras.h"

#include "VrToolbarStyle.h"
#include "VrToolbarCommands.h"

#include "ICardboardTetheringPlugin.h"

#include "LevelEditor.h"

static const FName VrToolbarTabName("VrToolbar");

#define LOCTEXT_NAMESPACE "FVrToolbarModule"

void FVrToolbarModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FVrToolbarStyle::Initialize();
	FVrToolbarStyle::ReloadTextures();

	FVrToolbarCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FVrToolbarCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FVrToolbarModule::PluginButtonClicked),
		FCanExecuteAction(),
    FGetActionCheckState::CreateRaw(this, &FVrToolbarModule::PluginButtonCheckState),
    EUIActionRepeatMode::RepeatDisabled);
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FVrToolbarModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FVrToolbarModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
}

void FVrToolbarModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FVrToolbarStyle::Shutdown();

	FVrToolbarCommands::Unregister();
}

void FVrToolbarModule::PluginButtonClicked() {
  if (ICardboardTetheringPlugin::Get().IsConnected()) {
    ICardboardTetheringPlugin::Get().Disconnect();
  } else {
    ICardboardTetheringPlugin::Get().ShowConnectDialog();
  }
}

ECheckBoxState FVrToolbarModule::PluginButtonCheckState() {
  if (ICardboardTetheringPlugin::Get().IsConnected()) {
    return ECheckBoxState::Checked;
  } else {
    return ECheckBoxState::Unchecked;
  }
}

void FVrToolbarModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FVrToolbarCommands::Get().PluginAction);
}

void FVrToolbarModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FVrToolbarCommands::Get().PluginAction);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVrToolbarModule, VrToolbar)