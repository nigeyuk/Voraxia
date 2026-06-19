// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoraxiaCameraZone.generated.h"

class UBoxComponent;
class UCurveFloat;
class UVoraxiaCameraComponent;

UCLASS(Blueprintable)
class VORAXIACAMERA_API AVoraxiaCameraZone : public AActor
{
	GENERATED_BODY()

public:
	AVoraxiaCameraZone();

protected:
	virtual void BeginPlay() override;

	/** Box trigger used to detect when a pawn enters or exits this camera zone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Setup")
	TObjectPtr<UBoxComponent> ZoneBounds;

	/** Only applies the zone to player-controlled pawns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Rules")
	bool bOnlyAffectPlayerControlledPawns = true;

	/** Camera distance to blend to while inside this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Framing")
	float ZoneCameraDistance = 350.0f;

	/** Pivot height to blend to while inside this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Framing")
	float ZonePivotHeight = 70.0f;

	/** Runtime camera offset to blend to while inside this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Framing")
	FVector ZoneCameraOffset = FVector::ZeroVector;

	/** Runtime pivot offset to blend to while inside this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Framing")
	FVector ZonePivotOffset = FVector::ZeroVector;

	/** Runtime FOV offset to blend to while inside this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Framing")
	float ZoneFOVOffset = 0.0f;

	/** Time, in seconds, used when blending into the zone framing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Blend", meta=(ClampMin="0.0"))
	float BlendInTime = 0.35f;

	/** Time, in seconds, used when resetting framing after leaving the zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Blend", meta=(ClampMin="0.0"))
	float BlendOutTime = 0.35f;

	/** Optional curve used for both blend in and blend out framing changes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Blend")
	TObjectPtr<UCurveFloat> BlendCurve;

	/** Resets the camera framing when the player leaves this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera Zone|Rules")
	bool bResetFramingOnExit = true;

private:
	UFUNCTION()
	void HandleZoneBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandleZoneEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	UVoraxiaCameraComponent* FindCameraComponent(AActor* Actor) const;
	bool ShouldAffectActor(const AActor* Actor) const;

	void ApplyZoneFraming(UVoraxiaCameraComponent* CameraComponent) const;
	void ResetZoneFraming(UVoraxiaCameraComponent* CameraComponent) const;
};
