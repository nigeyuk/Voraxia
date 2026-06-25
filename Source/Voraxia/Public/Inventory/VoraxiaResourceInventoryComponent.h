// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "VoraxiaResourceInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FVoraxiaResourceAmountChanged,
	FGameplayTag,
	ResourceTag,
	float,
	Delta,
	float,
	NewAmount
);

/*
 * Small, resource-only inventory used by the first Voraxia mining loop.
 *
 * It intentionally stores fractional amounts because voxel ore yields are
 * currently fractional. Capacity, mass, persistence, and replicated
 * fast-array networking are later stages, not hidden assumptions here.
 */
UCLASS(
	ClassGroup = (Voraxia),
	meta = (BlueprintSpawnableComponent)
)
class VORAXIA_API UVoraxiaResourceInventoryComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UVoraxiaResourceInventoryComponent();

	/*
	 * Adds a positive resource amount and returns the new stored total.
	 */
	UFUNCTION(BlueprintCallable, Category = "Voraxia|Inventory")
	float AddResource(FGameplayTag ResourceTag, float Amount);

	/*
	 * Removes an amount and returns the amount actually removed.
	 *
	 * If partial removal is disabled, this returns zero when the inventory
	 * cannot satisfy the complete request.
	 */
	UFUNCTION(BlueprintCallable, Category = "Voraxia|Inventory")
	float RemoveResource(
		FGameplayTag ResourceTag,
		float Amount,
		bool bAllowPartialRemoval = false
	);

	UFUNCTION(BlueprintPure, Category = "Voraxia|Inventory")
	float GetResourceAmount(FGameplayTag ResourceTag) const;

	UFUNCTION(BlueprintPure, Category = "Voraxia|Inventory")
	bool HasResource(FGameplayTag ResourceTag, float MinimumAmount = 0.01f) const;

	UFUNCTION(BlueprintCallable, Category = "Voraxia|Inventory")
	void ClearResource(FGameplayTag ResourceTag);

	UFUNCTION(BlueprintCallable, Category = "Voraxia|Inventory")
	void ClearAllResources();

	const TMap<FGameplayTag, float>& GetResourceAmounts() const
	{
		return ResourceAmounts;
	}

	UPROPERTY(BlueprintAssignable, Category = "Voraxia|Inventory")
	FVoraxiaResourceAmountChanged OnResourceAmountChanged;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Inventory", meta = (AllowPrivateAccess = "true"))
	TMap<FGameplayTag, float> ResourceAmounts;
};
