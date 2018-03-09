// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SkyboundGameMode.h"
#include "SkyboundPawn.h"
#include "SkyboundHud.h"

ASkyboundGameMode::ASkyboundGameMode()
{
	DefaultPawnClass = ASkyboundPawn::StaticClass();
	HUDClass = ASkyboundHud::StaticClass();
}
