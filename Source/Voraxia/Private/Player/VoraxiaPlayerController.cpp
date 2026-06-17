// Copyright 2026 Coding Custard Studios.

#include "Player/VoraxiaPlayerController.h"

#include "Settings/VoraxiaDeveloperSettings.h"
#include "VoraxiaLogCategories.h"

AVoraxiaPlayerController::AVoraxiaPlayerController()
{
	bShowMouseCursor = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AVoraxiaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameOnly());
	
	const UVoraxiaDeveloperSettings* Settings = UVoraxiaDeveloperSettings::Get();
	
	if (Settings && Settings->bEnableVoraxiaDebugging && Settings->bLogCharacterLifecycle)
	{
		UE_LOG(LogVoraxiaGame, Log, TEXT("Voraxia player Controller BeginPlay."));
	}
}

void AVoraxiaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// PlayerController-level input can go here later.
	// For movement/camera input, we will usually bind on the PlayerCharacter.
}