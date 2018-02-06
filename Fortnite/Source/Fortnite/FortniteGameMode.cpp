// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FortniteGameMode.h"
#include "FortniteHUD.h"
#include "FortniteCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFortniteGameMode::AFortniteGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AFortniteHUD::StaticClass();
}
