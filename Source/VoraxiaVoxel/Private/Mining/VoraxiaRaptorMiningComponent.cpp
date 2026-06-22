// Copyright 2026 Coding Custard Studios.

#include "Mining/VoraxiaRaptorMiningComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Mining/VoraxiaVoxelMineable.h"
#include "VoraxiaLog.h"

UVoraxiaRaptorMiningComponent::UVoraxiaRaptorMiningComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UVoraxiaRaptorMiningComponent::BeginPlay()
{
	Super::BeginPlay();

	bIsMining = false;
	MiningPulseAccumulator = 0.0f;

	SetComponentTickEnabled(false);
}

void UVoraxiaRaptorMiningComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(
		DeltaTime,
		TickType,
		ThisTickFunction
	);

	if (!bIsMining)
	{
		return;
	}

	const float SafePulseInterval = FMath::Max(
		MiningPulseInterval,
		0.02f
	);

	MiningPulseAccumulator += DeltaTime;

	if (MiningPulseAccumulator < SafePulseInterval)
	{
		return;
	}

	/*
	 * Do not catch up with multiple traces after a frame hitch.
	 * One carve pulse is enough, and it avoids a hitch becoming a sudden
	 * asteroid deletion event.
	 */
	MiningPulseAccumulator = 0.0f;

	PerformMiningTrace();
}

void UVoraxiaRaptorMiningComponent::StartMining()
{
	if (bIsMining)
	{
		return;
	}

	bIsMining = true;
	MiningPulseAccumulator = 0.0f;

	SetComponentTickEnabled(true);

	/*
	 * Immediate first response makes the tool feel responsive, then the
	 * component continues at MiningPulseInterval.
	 */
	PerformMiningTrace();

	UE_LOG(
		LogVoraxia,
		Log,
		TEXT("Raptor mining started on %s."),
		*GetOwner()->GetName()
	);
}

void UVoraxiaRaptorMiningComponent::StopMining()
{
	if (!bIsMining)
	{
		return;
	}

	bIsMining = false;
	MiningPulseAccumulator = 0.0f;

	SetComponentTickEnabled(false);

	UE_LOG(
		LogVoraxia,
		Log,
		TEXT("Raptor mining stopped on %s."),
		*GetOwner()->GetName()
	);
}

bool UVoraxiaRaptorMiningComponent::MineOnce()
{
	return PerformMiningTrace();
}

bool UVoraxiaRaptorMiningComponent::GetMiningViewPoint(
	FVector& OutLocation,
	FRotator& OutRotation
) const
{
	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return false;
	}

	if (const APlayerController* OwnerController = Cast<APlayerController>(Owner))
	{
		OwnerController->GetPlayerViewPoint(
			OutLocation,
			OutRotation
		);

		return true;
	}

	if (const APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		if (const APlayerController* PlayerController =
			Cast<APlayerController>(OwnerPawn->GetController()))
		{
			PlayerController->GetPlayerViewPoint(
				OutLocation,
				OutRotation
			);

			return true;
		}
	}

	/*
	 * Generic fallback for a non-player owner.
	 */
	Owner->GetActorEyesViewPoint(
		OutLocation,
		OutRotation
	);

	return true;
}

bool UVoraxiaRaptorMiningComponent::PerformMiningTrace()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	FVector TraceStart;
	FRotator TraceRotation;

	if (!GetMiningViewPoint(TraceStart, TraceRotation))
	{
		return false;
	}

	const FVector TraceEnd =
		TraceStart +
		TraceRotation.Vector() * FMath::Max(TraceDistance, 100.0f);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(VoraxiaRaptorMiningTrace),
		false,
		GetOwner()
	);

	QueryParams.bReturnPhysicalMaterial = false;

	FHitResult Hit;

	const bool bBlockingHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);

	if (bDrawDebugTrace)
	{
		const float DebugLifetime = 0.25f;

		const FColor TraceColour = bBlockingHit
			? FColor::Green
			: FColor::Red;

		const FVector DebugEnd = bBlockingHit
			? Hit.ImpactPoint
			: TraceEnd;

		DrawDebugLine(
			World,
			TraceStart,
			DebugEnd,
			TraceColour,
			false,
			DebugLifetime,
			0,
			2.0f
		);

		DrawDebugPoint(
			World,
			TraceStart,
			10.0f,
			FColor::Cyan,
			false,
			DebugLifetime
		);

		if (bBlockingHit)
		{
			DrawDebugSphere(
				World,
				Hit.ImpactPoint,
				CarveRadius,
				16,
				FColor::Yellow,
				false,
				DebugLifetime,
				0,
				1.25f
			);

			DrawDebugDirectionalArrow(
				World,
				Hit.ImpactPoint,
				Hit.ImpactPoint + Hit.ImpactNormal * 150.0f,
				25.0f,
				FColor::Blue,
				false,
				DebugLifetime,
				0,
				1.5f
			);
		}
	}

	if (!bBlockingHit)
	{
		return false;
	}
	
	AActor* HitActor = Hit.GetActor();
	
	UE_LOG(
	LogVoraxiaVoxel,
	Log,
	TEXT(
		"Raptor trace hit: Actor=%s | Component=%s | Impact=%s"
	),
	*GetNameSafe(HitActor),
	*GetNameSafe(Hit.GetComponent()),
	*Hit.ImpactPoint.ToString()
);

	if (
		!HitActor ||
		!HitActor->GetClass()->ImplementsInterface(
			UVoraxiaVoxelMineable::StaticClass()
		)
	)
	{
		return false;
	}

	return IVoraxiaVoxelMineable::Execute_ApplyVoxelCarve(
		HitActor,
		Hit,
		CarveRadius
	);
}