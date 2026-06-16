// Copyright 2026 Coding Custard Studios.


#include "Player/VoraxiaPlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AVoraxiaPlayerCharacter::AVoraxiaPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

		Movement->JumpZVelocity = 500.0f;
		Movement->AirControl = 0.35f;
		Movement->MaxWalkSpeed = 500.0f;
		Movement->MinAnalogWalkSpeed = 20.0f;
		Movement->BrakingDecelerationWalking = 2000.0f;
	}
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AVoraxiaPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AVoraxiaPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AVoraxiaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Enhanced Input bindings can go here once we create the Input Mapping Context and Actions.
}

