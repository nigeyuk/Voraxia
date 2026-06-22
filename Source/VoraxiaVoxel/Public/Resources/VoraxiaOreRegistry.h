// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "VoraxiaOreRegistry.generated.h"

class UVoraxiaOreDefinition;

/*
 * Central lookup asset for mineable resources.
 *
 * Asteroids store lightweight Gameplay Tags in their voxel-cell data.
 * Systems resolve that tag through this registry whenever they need the
 * ore's material, yield, colour, value, icon, or other definition data.
 */
UCLASS(BlueprintType)
class VORAXIAVOXEL_API UVoraxiaOreRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/*
	 * One entry for every ore currently available in the game.
	 *
	 * Do not add duplicate Ore Tags. The first matching definition will win
	 * during lookup, and duplicates are logged as a warning during validation.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore Registry",
		meta = (TitleProperty = "DisplayName")
	)
	TArray<TObjectPtr<UVoraxiaOreDefinition>> OreDefinitions;

	UFUNCTION(BlueprintPure, Category = "Voraxia|Ore Registry")
	const UVoraxiaOreDefinition* FindOreDefinition(
		FGameplayTag OreTag
	) const;

	UFUNCTION(BlueprintPure, Category = "Voraxia|Ore Registry")
	bool HasOreDefinition(
		FGameplayTag OreTag
	) const;
};
