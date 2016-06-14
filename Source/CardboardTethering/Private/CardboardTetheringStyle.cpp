// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CardboardTetheringPrivatePCH.h"

#include "CardboardTetheringStyle.h"
#include "SlateGameResources.h"
#include "IPluginManager.h"

TSharedPtr< FSlateStyleSet > CardboardTetheringStyle::StyleInstance = NULL;

void CardboardTetheringStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void CardboardTetheringStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName CardboardTetheringStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("CardboardTetheringStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define TTF_CORE_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::EngineContentDir()  / "Slate" / RelativePath + TEXT(".ttf"), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > CardboardTetheringStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("CardboardTetheringStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("CardboardTethering")->GetBaseDir() / TEXT("Resources"));

  Style->Set("CardboardTethering.StatusTitle", FTextBlockStyle()
    .SetFont(TTF_CORE_FONT("Fonts/Roboto-Regular", 16))
    .SetColorAndOpacity(FSlateColor::UseForeground())
    .SetShadowOffset(FVector2D::ZeroVector)
    .SetShadowColorAndOpacity(FLinearColor::Black));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void CardboardTetheringStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& CardboardTetheringStyle::Get()
{
	return *StyleInstance;
}
