// Copyright 2026 Coding Custard Studios.

#include "Mining/VoraxiaRaptorToolComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Mining/VoraxiaRaptorTarget.h"
#include "VoraxiaLog.h"

UVoraxiaRaptorToolComponent::UVoraxiaRaptorToolComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UVoraxiaRaptorToolComponent::BeginPlay()
{
	Super::BeginPlay();

	bIsFiring = bStartFiringAutomatically;

	UE_LOG(LogVoraxia, Log, TEXT("Raptor tool ready on %s"), *GetOwner()->GetName());
}

void UVoraxiaRaptorToolComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsFiring)
	{
		return;
	}

	FireRaptorTrace(DeltaTime);
}

void UVoraxiaRaptorToolComponent::StartFiring()
{
	bIsFiring = true;
}

void UVoraxiaRaptorToolComponent::StopFiring()
{
	bIsFiring = false;
}

bool UVoraxiaRaptorToolComponent::GetTraceViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(Owner);

	if (OwnerPawn)
	{
		const APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());

		if (PlayerController)
		{
			PlayerController->GetPlayerViewPoint(OutLocation, OutRotation);
			return true;
		}
	}

	OutLocation = Owner->GetActorLocation();
	OutRotation = Owner->GetActorRotation();
	return true;
}

void UVoraxiaRaptorToolComponent::FireRaptorTrace(float DeltaTime)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	FVector TraceStart;
	FRotator TraceRotation;

	if (!GetTraceViewPoint(TraceStart, TraceRotation))
	{
		return;
	}

	const FVector TraceEnd = TraceStart + TraceRotation.Vector() * TraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VoraxiaRaptorTrace), false);
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);

	if (bDrawDebugTrace)
	{
		const FColor TraceColor = bHit ? FColor::Green : FColor::Red;

		DrawDebugLine(
			World,
			TraceStart,
			bHit ? Hit.ImpactPoint : TraceEnd,
			TraceColor,
			false,
			0.0f,
			0,
			1.5f
		);

		if (bHit)
		{
			DrawDebugSphere(
				World,
				Hit.ImpactPoint,
				12.0f,
				12,
				FColor::Yellow,
				false,
				0.0f
			);
		}
	}

	if (!bHit)
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();

	if (!HitActor)
	{
		return;
	}

	if (HitActor->GetClass()->ImplementsInterface(UVoraxiaRaptorTarget::StaticClass()))
	{
		IVoraxiaRaptorTarget::Execute_ApplyRaptorMining(
			HitActor,
			Hit,
			MiningPower,
			DeltaTime
		);
	}
}
