// Copyright 2026 Coding Custard Studios.

#include "Player/VoraxiaPlayerController.h"

AVoraxiaPlayerController::AVoraxiaPlayerController()
{
	bShowMouseCursor = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AVoraxiaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameOnly());
}

void AVoraxiaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// PlayerController-level input can go here later.
	// For movement/camera input, we will usually bind on the PlayerCharacter.
}