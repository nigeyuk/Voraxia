// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mining/VoraxiaRaptorTarget.h"
#include "VoraxiaDissolveAsteroidActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FVoraxiaDissolveWound
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve")
	bool bActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve")
	FVector LocalPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve")
	float Radius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve")
	float Strength = 0.0f;
};

UCLASS(Blueprintable)
class VORAXIA_API AVoraxiaDissolveAsteroidActor : public AActor, public IVoraxiaRaptorTarget
{
	GENERATED_BODY()

public:
	AVoraxiaDissolveAsteroidActor();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	virtual void ApplyRaptorMining_Implementation(const FHitResult& Hit, float MiningPower, float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Asteroid", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AsteroidMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve")
	int32 MaxWounds = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve", meta = (ClampMin = "1.0"))
	float InitialWoundRadius = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve", meta = (ClampMin = "1.0"))
	float RadiusGrowthPerSecond = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve", meta = (ClampMin = "0.01"))
	float StrengthGrowthPerSecond = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve", meta = (ClampMin = "1.0"))
	float WoundMergeDistance = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve")
	bool bDisableCollisionWhenFullyMined = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Dissolve")
	TArray<FVoraxiaDissolveWound> Wounds;

protected:
	void CreateDynamicMaterial();
	void EnsureWoundSlots();
	int32 FindBestWoundSlot(const FVector& LocalPosition) const;
	void UpdateDissolveMaterialParameters();
	float GetTotalDissolveStrength() const;

public:
	UFUNCTION(BlueprintCallable, Category = "Voraxia|Asteroid")
	UStaticMeshComponent* GetAsteroidMesh() const { return AsteroidMesh; }
};
