// Copyright 2026 Coding Custard Studios.


#include "Player/VoraxiaPlayerCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"

AVoraxiaPlayerCharacterBase::AVoraxiaPlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AVoraxiaPlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

