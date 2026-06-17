// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoraxiaRaptorToolComponent.generated.h"

UCLASS(ClassGroup = (Voraxia), meta = (BlueprintSpawnableComponent))
class VORAXIA_API UVoraxiaRaptorToolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoraxiaRaptorToolComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Voraxia|Raptor")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category = "Voraxia|Raptor")
	void StopFiring();

	UFUNCTION(BlueprintCallable, Category = "Voraxia|Raptor")
	bool IsFiring() const { return bIsFiring; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Raptor", meta = (ClampMin = "100.0"))
	float TraceDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Raptor", meta = (ClampMin = "0.0"))
	float MiningPower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Raptor")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Raptor")
	bool bStartFiringAutomatically = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Debug")
	bool bDrawDebugTrace = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Raptor")
	bool bIsFiring = false;

protected:
	bool GetTraceViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
	void FireRaptorTrace(float DeltaTime);
};