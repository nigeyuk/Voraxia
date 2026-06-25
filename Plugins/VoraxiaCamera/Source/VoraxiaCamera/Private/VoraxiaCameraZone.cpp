// Copyright 2026 Coding Custard Studios.

#include "VoraxiaCameraZone.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "VoraxiaCameraComponent.h"
#include "VoraxiaCameraLog.h"

AVoraxiaCameraZone::AVoraxiaCameraZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
	SetRootComponent(ZoneBounds);

	ZoneBounds->InitBoxExtent(FVector(300.0f, 300.0f, 200.0f));
	ZoneBounds->SetCollisionProfileName(TEXT("Trigger"));
	ZoneBounds->SetGenerateOverlapEvents(true);
}

void AVoraxiaCameraZone::BeginPlay()
{
	Super::BeginPlay();

	if (!ZoneBounds)
	{
		return;
	}

	ZoneBounds->OnComponentBeginOverlap.AddDynamic(
		this,
		&AVoraxiaCameraZone::HandleZoneBeginOverlap
	);

	ZoneBounds->OnComponentEndOverlap.AddDynamic(
		this,
		&AVoraxiaCameraZone::HandleZoneEndOverlap
	);
}

void AVoraxiaCameraZone::HandleZoneBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	UE_LOG(
		LogVoraxiaCamera,
		Warning,
		TEXT("Camera zone '%s' begin overlap with actor '%s', component '%s'."),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		*GetNameSafe(OtherComp)
	);

	if (!ShouldAffectActor(OtherActor))
	{
		UE_LOG(
			LogVoraxiaCamera,
			Warning,
			TEXT("Camera zone '%s' rejected actor '%s'."),
			*GetNameSafe(this),
			*GetNameSafe(OtherActor)
		);

		return;
	}

	UVoraxiaCameraComponent* CameraComponent = FindCameraComponent(OtherActor);

	if (!CameraComponent)
	{
		UE_LOG(
			LogVoraxiaCamera,
			Warning,
			TEXT("Camera zone '%s' could not find VoraxiaCameraComponent on '%s'."),
			*GetNameSafe(this),
			*GetNameSafe(OtherActor)
		);

		return;
	}

	ApplyZoneFraming(CameraComponent);
}

void AVoraxiaCameraZone::HandleZoneEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	UE_LOG(
		LogVoraxiaCamera,
		Warning,
		TEXT("Camera zone '%s' end overlap with actor '%s', component '%s'."),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		*GetNameSafe(OtherComp)
	);

	if (!bResetFramingOnExit || !ShouldAffectActor(OtherActor))
	{
		return;
	}

	UVoraxiaCameraComponent* CameraComponent = FindCameraComponent(OtherActor);

	if (!CameraComponent)
	{
		return;
	}

	ResetZoneFraming(CameraComponent);
}

UVoraxiaCameraComponent* AVoraxiaCameraZone::FindCameraComponent(AActor* Actor) const
{
	if (!Actor)
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<UVoraxiaCameraComponent>();
}

bool AVoraxiaCameraZone::ShouldAffectActor(const AActor* Actor) const
{
	const APawn* Pawn = Cast<APawn>(Actor);

	/*
	 * Camera zones are local presentation. On a server, every connected
	 * player Pawn may be player-controlled, but only the locally viewed Pawn
	 * should have its camera framing changed on this machine.
	 */
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return false;
	}

	return !bOnlyAffectPlayerControlledPawns || Pawn->IsPlayerControlled();
}

void AVoraxiaCameraZone::ApplyZoneFraming(UVoraxiaCameraComponent* CameraComponent) const
{
	if (!CameraComponent)
	{
		return;
	}

	CameraComponent->SetFraming(
		ZoneCameraDistance,
		ZonePivotHeight,
		ZoneCameraOffset,
		ZonePivotOffset,
		ZoneFOVOffset,
		BlendInTime,
		BlendCurve
	);

	UE_LOG(
		LogVoraxiaCamera,
		Log,
		TEXT("Voraxia camera zone '%s' applied framing to '%s'."),
		*GetNameSafe(this),
		*GetNameSafe(CameraComponent->GetOwner())
	);
}

void AVoraxiaCameraZone::ResetZoneFraming(UVoraxiaCameraComponent* CameraComponent) const
{
	if (!CameraComponent)
	{
		return;
	}

	CameraComponent->ResetFraming(
		BlendOutTime,
		BlendCurve
	);

	UE_LOG(
		LogVoraxiaCamera,
		Log,
		TEXT("Voraxia camera zone '%s' reset framing on '%s'."),
		*GetNameSafe(this),
		*GetNameSafe(CameraComponent->GetOwner())
	);
}
