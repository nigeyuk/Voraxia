// Copyright 2026 Coding Custard Studios.

#include "Player/VoraxiaPlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Player/VoraxiaCharacterMovementComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "DrawDebugHelpers.h"

#include "VoraxiaInteractableInterface.h"
#include "Camera/CameraComponent.h"
#include "VoraxiaCameraComponent.h"
#include "Mining/VoraxiaRaptorMiningComponent.h"
#include "Mining/VoraxiaMiningTypes.h"
#include "Inventory/VoraxiaResourceInventoryComponent.h"
#include "VoraxiaCameraOcclusionDitherComponent.h"
#include "UI/SVoraxiaMiningLedgerWidget.h"
#include "VoraxiaBlueprintDataAsset.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Net/UnrealNetwork.h"

AVoraxiaPlayerCharacter::AVoraxiaPlayerCharacter(
	const FObjectInitializer& ObjectInitializer
)
	: Super(
		ObjectInitializer.SetDefaultSubobjectClass<
			UVoraxiaCharacterMovementComponent
		>(ACharacter::CharacterMovementComponentName)
	)
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

	VoraxiaCameraComponent =
	CreateDefaultSubobject<UVoraxiaCameraComponent>
		(TEXT("VoraxiaCameraComponent"));

	RaptorMiningComponent =
	CreateDefaultSubobject<UVoraxiaRaptorMiningComponent>(
		TEXT("RaptorMiningComponent")
	);

	ResourceInventoryComponent =
		CreateDefaultSubobject<UVoraxiaResourceInventoryComponent>(
			TEXT("ResourceInventoryComponent")
		);

	CameraOcclusionDitherComponent =
		CreateDefaultSubobject<UVoraxiaCameraOcclusionDitherComponent>(
			TEXT("CameraOcclusionDitherComponent")
		);
}

void AVoraxiaPlayerCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/*
	 * Physical blueprints are personal inventory. Only the owning client needs
	 * the list, while the server remains the single authority that changes it.
	 */
	DOREPLIFETIME_CONDITION(
		AVoraxiaPlayerCharacter,
		PhysicalBlueprints,
		COND_OwnerOnly
	);
}

void AVoraxiaPlayerCharacter::ReceiveMiningYield_Implementation(
	const FVoraxiaMiningYield& MiningYield
)
{
	if (
		!MiningYield.ResourceTag.IsValid() ||
		MiningYield.Amount <= KINDA_SMALL_NUMBER ||
		!ResourceInventoryComponent
	)
	{
		return;
	}

	const float NewTotal = ResourceInventoryComponent->AddResource(
		MiningYield.ResourceTag,
		MiningYield.Amount
	);

	if (bLogMiningLedgerUpdates)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"Mining inventory updated: Player=%s | Resource=%s | "
				"Added=%.2f | Total=%.2f"
			),
			*GetName(),
			*MiningYield.ResourceTag.ToString(),
			MiningYield.Amount,
			NewTotal
		);
	}
}

float AVoraxiaPlayerCharacter::GetMinedResourceAmount(
	const FGameplayTag ResourceTag
) const
{
	return ResourceInventoryComponent
		? ResourceInventoryComponent->GetResourceAmount(ResourceTag)
		: 0.0f;
}

const TMap<FGameplayTag, float>&
AVoraxiaPlayerCharacter::GetResourceInventory() const
{
	static const TMap<FGameplayTag, float> EmptyInventory;

	return ResourceInventoryComponent
		? ResourceInventoryComponent->GetResourceAmounts()
		: EmptyInventory;
}

void AVoraxiaPlayerCharacter::LogMiningLedger() const
{
	const TMap<FGameplayTag, float>& Inventory =
		GetResourceInventory();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Mining inventory: %s | Resource Types=%d"),
		*GetName(),
		Inventory.Num()
	);

	for (const TPair<FGameplayTag, float>& Pair : Inventory)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Mining inventory entry: %s | Total=%.2f"),
			*Pair.Key.ToString(),
			Pair.Value
		);
	}
}

void AVoraxiaPlayerCharacter::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*
	 * Locally controlled characters retain the original smooth sprint response.
	 * Remote-player server copies update their speed inside MoveAutonomous using
	 * the sprint flag carried by the matching saved movement move.
	 */
	if (IsLocallyControlled())
	{
		UpdateSprintSpeed(DeltaTime);
	}

	UpdateCharacterFacing(DeltaTime);
}

void AVoraxiaPlayerCharacter::UpdateCharacterFacing(const float DeltaTime)
{
	/*
	 * Preserve Voraxia's existing camera-facing body behaviour on the owning
	 * machine. The custom movement component only records this final yaw so the
	 * server can replay the matching value within the normal saved-move packet.
	 */
	if (
		!IsLocallyControlled() ||
		!bCharacterFacesCameraYaw ||
		!CameraComponent
	)
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

	if (UVoraxiaCharacterMovementComponent* VoraxiaMovement =
		Cast<UVoraxiaCharacterMovementComponent>(GetCharacterMovement()))
	{
		VoraxiaMovement->SetVoraxiaFacingYaw(NewRotation.Yaw);
	}
}

void AVoraxiaPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	AddDefaultMappingContext();
	AddMiningLedgerWidget();

	if (VoraxiaCameraComponent && CameraComponent)
	{
		VoraxiaCameraComponent->SetTargetCamera(CameraComponent);
	}

	if (CameraOcclusionDitherComponent)
	{
		CameraOcclusionDitherComponent->SetCameraComponent(VoraxiaCameraComponent);
	}
}


void AVoraxiaPlayerCharacter::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	RemoveMiningLedgerWidget();

	Super::EndPlay(EndPlayReason);
}

bool AVoraxiaPlayerCharacter::ShouldCreateMiningLedgerWidget() const
{
	if (MiningLedgerAllowedMap.IsNull())
	{
		return false;
	}

	const UWorld* CurrentWorld = GetWorld();

	if (!CurrentWorld)
	{
		return false;
	}

	const FString AllowedMapPackageName =
		MiningLedgerAllowedMap.ToSoftObjectPath().GetLongPackageName();

	if (AllowedMapPackageName.IsEmpty())
	{
		return false;
	}

	/*
	 * PIE gives each play session an instanced package name such as
	 * UEDPIE_0_MiningTest. Remove that prefix before comparing it against the
	 * selected map asset, so the rule behaves the same in PIE and a packaged
	 * build.
	 */
	const FString CurrentMapPackageName = UWorld::RemovePIEPrefix(
		CurrentWorld->GetOutermost()->GetName(),
		nullptr
	);

	return CurrentMapPackageName == AllowedMapPackageName;
}

void AVoraxiaPlayerCharacter::AddMiningLedgerWidget()
{
	if (
		!IsLocallyControlled() ||
		!ShouldCreateMiningLedgerWidget() ||
		MiningLedgerWidget.IsValid() ||
		!GEngine ||
		!GEngine->GameViewport
	)
	{
		return;
	}

	SAssignNew(
		MiningLedgerWidget,
		SVoraxiaMiningLedgerWidget
	)
	.OwningPlayer(this);

	GEngine->GameViewport->AddViewportWidgetContent(
		MiningLedgerWidget.ToSharedRef(),
		20
	);
}

void AVoraxiaPlayerCharacter::RemoveMiningLedgerWidget()
{
	if (
		MiningLedgerWidget.IsValid() &&
		GEngine &&
		GEngine->GameViewport
	)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(
			MiningLedgerWidget.ToSharedRef()
		);
	}

	MiningLedgerWidget.Reset();
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

	if (SwapShoulderAction)
	{
		EnhancedInputComponent->BindAction(
			SwapShoulderAction,
			ETriggerEvent::Started,
			this,
			&AVoraxiaPlayerCharacter::SwapShoulderStarted
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia SwapShoulderAction is not assigned."));
	}

	if (InteractAction)
	{
		EnhancedInputComponent->BindAction(
			InteractAction,
			ETriggerEvent::Started,
			this,
			&AVoraxiaPlayerCharacter::TryInteract
	);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia InteractAction is not assigned."));
	}

	if (MiningAction)
	{
		EnhancedInputComponent->BindAction(
			MiningAction,
			ETriggerEvent::Started,
			this,
			&AVoraxiaPlayerCharacter::MiningStarted
		);

		EnhancedInputComponent->BindAction(
			MiningAction,
			ETriggerEvent::Completed,
			this,
			&AVoraxiaPlayerCharacter::MiningEnded
		);

		EnhancedInputComponent->BindAction(
			MiningAction,
			ETriggerEvent::Canceled,
			this,
			&AVoraxiaPlayerCharacter::MiningEnded
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Voraxia MiningAction is not assigned."));
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

bool AVoraxiaPlayerCharacter::IsSprintRequested() const
{
	return bWantsToSprint && bCanSprint;
}

void AVoraxiaPlayerCharacter::RestorePredictedSprintIntent(
	const bool bShouldSprint
)
{
	bWantsToSprint = bShouldSprint && bCanSprint;
}

void AVoraxiaPlayerCharacter::ApplyNetworkSprintIntent(
	const bool bShouldSprint,
	const float DeltaTime
)
{
	RestorePredictedSprintIntent(bShouldSprint);
	UpdateSprintSpeed(DeltaTime);
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

void AVoraxiaPlayerCharacter::SwapShoulderStarted(
	const FInputActionValue& Value
)
{
	if (!VoraxiaCameraComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("VoraxiaCameraComponent is null when swapping shoulder.")
		);

		return;
	}

	VoraxiaCameraComponent->SwapCameraShoulder(0.25f);
}

void AVoraxiaPlayerCharacter::MiningStarted(
	const FInputActionValue& Value
)
{
	if (!RaptorMiningComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("RaptorMiningComponent is null when mining starts.")
		);

		return;
	}

	RaptorMiningComponent->StartMining();
}

void AVoraxiaPlayerCharacter::MiningEnded(
	const FInputActionValue& Value
)
{
	if (!RaptorMiningComponent)
	{
		return;
	}

	RaptorMiningComponent->StopMining();
}

bool AVoraxiaPlayerCharacter::AddPhysicalBlueprint(
	UVoraxiaBlueprintDataAsset* BlueprintData
)
{
	if (!HasAuthority() || !BlueprintData)
	{
		return false;
	}

	if (PhysicalBlueprints.Contains(BlueprintData))
	{
		return false;
	}

	PhysicalBlueprints.Add(BlueprintData);
	ForceNetUpdate();
	return true;
}

bool AVoraxiaPlayerCharacter::RemovePhysicalBlueprint(
	UVoraxiaBlueprintDataAsset* BlueprintData
)
{
	if (!HasAuthority() || !BlueprintData)
	{
		return false;
	}

	const bool bRemoved = PhysicalBlueprints.Remove(BlueprintData) > 0;

	if (bRemoved)
	{
		ForceNetUpdate();
	}

	return bRemoved;
}

bool AVoraxiaPlayerCharacter::HasPhysicalBlueprint(
	UVoraxiaBlueprintDataAsset* BlueprintData
) const
{
	return BlueprintData && PhysicalBlueprints.Contains(BlueprintData);
}

const TArray<TObjectPtr<UVoraxiaBlueprintDataAsset>>&
AVoraxiaPlayerCharacter::GetPhysicalBlueprints() const
{
	return PhysicalBlueprints;
}

UVoraxiaBlueprintDataAsset*
AVoraxiaPlayerCharacter::GetFirstPhysicalBlueprint() const
{
	return PhysicalBlueprints.Num() > 0 ? PhysicalBlueprints[0] : nullptr;
}

void AVoraxiaPlayerCharacter::OnRep_PhysicalBlueprints()
{
	/*
	 * SVoraxiaMiningLedgerWidget reads GetPhysicalBlueprints() through a Slate
	 * text binding, so it refreshes naturally on the next paint. This hook is
	 * intentionally kept for later inventory UI events and sound feedback.
	 */
}

void AVoraxiaPlayerCharacter::TryInteract()
{
	UE_LOG(LogTemp, Verbose, TEXT("Voraxia interaction request started."));

	if (!CameraComponent || !GetWorld())
	{
		return;
	}

	const FVector TraceStart = CameraComponent->GetComponentLocation();
	const FVector TraceEnd = TraceStart +
		(CameraComponent->GetForwardVector() * InteractionTraceDistance);

	DrawDebugLine(
		GetWorld(),
		TraceStart,
		TraceEnd,
		FColor::Green,
		false,
		2.0f,
		0,
		2.0f
	);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(VoraxiaInteractionTrace),
		false,
		this
	);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	AActor* HitActor = bHit ? HitResult.GetActor() : nullptr;

	if (
		!HitActor ||
		!HitActor->GetClass()->ImplementsInterface(
			UVoraxiaInteractableInterface::StaticClass()
		)
	)
	{
		return;
	}

	/*
	 * The local trace is an intent selector only. The server independently
	 * validates range and line of sight, then performs the state mutation.
	 */
	if (HasAuthority())
	{
		TryInteractServerAuthoritative(HitActor);
	}
	else
	{
		ServerTryInteract(HitActor);
	}
}

void AVoraxiaPlayerCharacter::ServerTryInteract_Implementation(
	AActor* RequestedActor
)
{
	TryInteractServerAuthoritative(RequestedActor);
}

void AVoraxiaPlayerCharacter::TryInteractServerAuthoritative(
	AActor* RequestedActor
)
{
	if (!HasAuthority() || !IsInteractionTargetValid(RequestedActor))
	{
		return;
	}

	IVoraxiaInteractableInterface::Execute_Interact(
		RequestedActor,
		this
	);
}

bool AVoraxiaPlayerCharacter::IsInteractionTargetValid(
	const AActor* InteractionTarget
) const
{
	if (
		!InteractionTarget ||
		InteractionTarget == this ||
		!InteractionTarget->GetClass()->ImplementsInterface(
			UVoraxiaInteractableInterface::StaticClass()
		) ||
		!GetWorld()
	)
	{
		return false;
	}

	FVector TargetOrigin = FVector::ZeroVector;
	FVector TargetExtent = FVector::ZeroVector;
	InteractionTarget->GetActorBounds(
		true,
		TargetOrigin,
		TargetExtent
	);

	const FVector PlayerLocation = GetActorLocation();
	const FVector ClosestTargetPoint(
		FMath::Clamp(
			PlayerLocation.X,
			TargetOrigin.X - TargetExtent.X,
			TargetOrigin.X + TargetExtent.X
		),
		FMath::Clamp(
			PlayerLocation.Y,
			TargetOrigin.Y - TargetExtent.Y,
			TargetOrigin.Y + TargetExtent.Y
		),
		FMath::Clamp(
			PlayerLocation.Z,
			TargetOrigin.Z - TargetExtent.Z,
			TargetOrigin.Z + TargetExtent.Z
		)
	);

	const float MaximumDistance = FMath::Max(
		0.0f,
		InteractionTraceDistance + InteractionServerValidationTolerance
	);

	if (
		FVector::DistSquared(PlayerLocation, ClosestTargetPoint) >
		FMath::Square(MaximumDistance)
	)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(VoraxiaServerInteractionValidation),
		false,
		this
	);

	FHitResult VisibilityHit;
	const FVector TraceStart = PlayerLocation +
		FVector(0.0f, 0.0f, BaseEyeHeight);

	if (
		GetWorld()->LineTraceSingleByChannel(
			VisibilityHit,
			TraceStart,
			ClosestTargetPoint,
			ECC_Visibility,
			QueryParams
		) &&
		VisibilityHit.GetActor() != InteractionTarget
	)
	{
		return false;
	}

	return true;
}
