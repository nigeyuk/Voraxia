// Copyright 2026 Coding Custard Studios.

#include "Player/VoraxiaPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "VoraxiaCameraComponent.h"

AVoraxiaPlayerCharacter::AVoraxiaPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	if (MovementComponent)
	{
		MovementComponent->bOrientRotationToMovement = false;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		MovementComponent->MaxWalkSpeed = WalkSpeed;
		MovementComponent->JumpZVelocity = 500.0f;
		MovementComponent->AirControl = 0.35f;
	}

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->bUsePawnControlRotation = false;

	VoraxiaCameraComponent = CreateDefaultSubobject<UVoraxiaCameraComponent>(TEXT("VoraxiaCameraComponent"));
}

void AVoraxiaPlayerCharacter::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateSprintSpeed(DeltaTime);
	UpdateCharacterFacing(DeltaTime);
}

void AVoraxiaPlayerCharacter::UpdateCharacterFacing(const float DeltaTime)
{
	if (!bCharacterFacesCameraYaw || !CameraComponent)
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();

	const FRotator TargetRotation(
		0.0f,
		CameraComponent->GetComponentRotation().Yaw,
		0.0f
	);

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		FaceCameraYawInterpSpeed
	);

	SetActorRotation(NewRotation);
}

void AVoraxiaPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	AddDefaultMappingContext();

	if (VoraxiaCameraComponent && CameraComponent)
	{
		VoraxiaCameraComponent->SetTargetCamera(CameraComponent);
	}
}

void AVoraxiaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia player input component is not an Enhanced Input Component."));
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&AVoraxiaPlayerCharacter::Move
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia MoveAction is not assigned."));
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&AVoraxiaPlayerCharacter::Look
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia LookAction is not assigned."));
	}

	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Started,
			this,
			&ACharacter::Jump
		);

		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Completed,
			this,
			&ACharacter::StopJumping
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia JumpAction is not assigned."));
	}

	if (SprintAction)
	{
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Started,
			this,
			&AVoraxiaPlayerCharacter::SprintStarted
		);

		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Completed,
			this,
			&AVoraxiaPlayerCharacter::SprintEnded
		);

		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Canceled,
			this,
			&AVoraxiaPlayerCharacter::SprintEnded
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia SprintAction is not assigned."));
	}

	if (FocusAction)
	{
		EnhancedInputComponent->BindAction(
			FocusAction,
			ETriggerEvent::Started,
			this,
			&AVoraxiaPlayerCharacter::FocusStarted
		);

		EnhancedInputComponent->BindAction(
			FocusAction,
			ETriggerEvent::Completed,
			this,
			&AVoraxiaPlayerCharacter::FocusEnded
		);

		EnhancedInputComponent->BindAction(
			FocusAction,
			ETriggerEvent::Canceled,
			this,
			&AVoraxiaPlayerCharacter::FocusEnded
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia FocusAction is not assigned."));
	}
}

void AVoraxiaPlayerCharacter::AddDefaultMappingContext()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (!PlayerController || !DefaultMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	if (!InputSubsystem)
	{
		return;
	}

	InputSubsystem->RemoveMappingContext(DefaultMappingContext);
	InputSubsystem->AddMappingContext(DefaultMappingContext, MappingContextPriority);
}

void AVoraxiaPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementValue = Value.Get<FVector2D>();

	if (!Controller)
	{
		return;
	}

	const FRotator CameraRotation =
		CameraComponent
			? CameraComponent->GetComponentRotation()
			: Controller->GetControlRotation();

	const FRotator YawRotation(0.0f, CameraRotation.Yaw, 0.0f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementValue.Y);
	AddMovementInput(RightDirection, MovementValue.X);
}

void AVoraxiaPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();

	if (!VoraxiaCameraComponent)
	{
		return;
	}

	VoraxiaCameraComponent->AddYawInput(LookValue.X);
	VoraxiaCameraComponent->AddPitchInput(LookValue.Y);
}

void AVoraxiaPlayerCharacter::SprintStarted(const FInputActionValue& Value)
{
	if (!bCanSprint)
	{
		return;
	}

	bWantsToSprint = true;
}

void AVoraxiaPlayerCharacter::SprintEnded(const FInputActionValue& Value)
{
	bWantsToSprint = false;
}

void AVoraxiaPlayerCharacter::UpdateSprintSpeed(const float DeltaTime)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	if (!MovementComponent)
	{
		return;
	}

	const float TargetSpeed = bWantsToSprint && bCanSprint
		? SprintSpeed
		: WalkSpeed;

	if (SprintSpeedInterpSpeed <= KINDA_SMALL_NUMBER || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		MovementComponent->MaxWalkSpeed = TargetSpeed;
		return;
	}

	MovementComponent->MaxWalkSpeed = FMath::FInterpTo(
		MovementComponent->MaxWalkSpeed,
		TargetSpeed,
		DeltaTime,
		SprintSpeedInterpSpeed
	);
}

void AVoraxiaPlayerCharacter::FocusStarted(const FInputActionValue& Value)
{
	if (!VoraxiaCameraComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoraxiaCameraComponent is null."));
		return;
	}

	if (FocusInputMode == EVoraxiaFocusInputMode::Toggle)
	{
		bFocusToggleActive = !bFocusToggleActive;

		if (bFocusToggleActive)
		{
			VoraxiaCameraComponent->FocusDefaultTaggedActor(FocusBlendInTime);
		}
		else
		{
			VoraxiaCameraComponent->ClearFocus(FocusBlendOutTime);
		}

		return;
	}

	VoraxiaCameraComponent->FocusDefaultTaggedActor(FocusBlendInTime);
}

void AVoraxiaPlayerCharacter::FocusEnded(const FInputActionValue& Value)
{
	if (FocusInputMode == EVoraxiaFocusInputMode::Toggle)
	{
		return;
	}

	if (!VoraxiaCameraComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoraxiaCameraComponent is null when clearing focus."));
		return;
	}

	bFocusToggleActive = false;
	VoraxiaCameraComponent->ClearFocus(FocusBlendOutTime);
}

