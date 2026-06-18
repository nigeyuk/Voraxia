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
		MovementComponent->MaxWalkSpeed = 500.0f;
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

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&AVoraxiaPlayerCharacter::Look
		);
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