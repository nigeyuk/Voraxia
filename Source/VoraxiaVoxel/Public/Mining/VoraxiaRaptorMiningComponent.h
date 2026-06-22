// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoraxiaRaptorMiningComponent.generated.h"

/*
 * Camera-view mining tool component.
 *
 * While mining is active, this traces from the owning player's current view
 * and asks any IVoraxiaVoxelMineable target to apply a real voxel carve.
 *
 * It deliberately performs mining in timed pulses rather than every frame:
 * the current voxel test sphere remeshes and refreshes collision after each
 * density edit, so a controlled pulse rate keeps the prototype responsive.
 */
UCLASS(
	ClassGroup = (VoraxiaVoxel),
	meta = (BlueprintSpawnableComponent)
)
class VORAXIAVOXEL_API UVoraxiaRaptorMiningComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoraxiaRaptorMiningComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UFUNCTION(BlueprintCallable, Category = "Voraxia|Mining")
	void StartMining();

	UFUNCTION(BlueprintCallable, Category = "Voraxia|Mining")
	void StopMining();

	UFUNCTION(BlueprintCallable, Category = "Voraxia|Mining")
	bool MineOnce();

	UFUNCTION(BlueprintPure, Category = "Voraxia|Mining")
	bool IsMining() const
	{
		return bIsMining;
	}

protected:
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Mining",
		meta = (ClampMin = "100.0")
	)
	float TraceDistance = 7500.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Mining",
		meta = (ClampMin = "1.0")
	)
	float CarveRadius = 100.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Mining",
		meta = (ClampMin = "0.02", ClampMax = "1.0")
	)
	float MiningPulseInterval = 0.12f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Mining"
	)
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Mining|Debug"
	)
	bool bDrawDebugTrace = false;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Mining"
	)
	bool bIsMining = false;

private:
	float MiningPulseAccumulator = 0.0f;

	bool GetMiningViewPoint(
		FVector& OutLocation,
		FRotator& OutRotation
	) const;

	bool PerformMiningTrace();
};