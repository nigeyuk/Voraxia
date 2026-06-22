// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "VoraxiaOreDefinition.generated.h"

class UMaterialInterface;
class UTexture2D;

/*
 * Defines everything gameplay and presentation systems need to know about
 * one mineable material.
 *
 * Gameplay Tag:
 *     Identifies the ore everywhere in code, inventories, asteroids,
 *     exchanges, contracts, scanners, and construction recipes.
 *
 * Data Asset:
 *     Defines how that ore looks, how valuable it is, and how much material
 *     is awarded when a voxel cell is fully vaporised.
 */
UCLASS(BlueprintType)
class VORAXIAVOXEL_API UVoraxiaOreDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/*
	 * Stable identity for this ore.
	 *
	 * Example:
	 * Resource.Ore.Iron
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Identity",
		meta = (Categories = "Resource.Ore")
	)
	FGameplayTag OreTag;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Identity"
	)
	FText DisplayName;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Identity",
		meta = (MultiLine = "true")
	)
	FText Description;

	/*
	 * Material used when this ore is exposed on an asteroid surface.
	 *
	 * For the MVP this may be a simple flat-colour Material Instance.
	 * Later it can become a layer inside a more advanced asteroid material.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Presentation"
	)
	TObjectPtr<UMaterialInterface> AsteroidSurfaceMaterial;

	/*
	 * Prototype/debug colour used by ore-cell debug drawing, scanner pings,
	 * temporary UI, and simple surface-vein visualisation.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Presentation"
	)
	FLinearColor DebugColour = FLinearColor::White;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Presentation"
	)
	TObjectPtr<UTexture2D> InventoryIcon;

	/*
	 * Amount awarded when an ore voxel cell is fully vaporised.
	 *
	 * The first implementation awards whole-cell yields. Later systems can
	 * support partial yield based on actual removed density volume.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voraxia|Ore|Mining",
	meta = (ClampMin = "0.01", UIMin = "0.01", UIMax = "10.0"))
	float YieldPerVoxelCell = 1.0f;

	/*
	 * A simple multiplier used by mining tools when determining how quickly
	 * this material can be removed.
	 *
	 * 1.0 = normal
	 * Below 1.0 = easier to vaporise
	 * Above 1.0 = tougher to vaporise
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Mining",
		meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "10.0")
	)
	float MiningResistance = 1.0f;

	/*
	 * Initial station-exchange reference value per yielded unit.
	 *
	 * This is not a full market simulation. It is a safe early baseline
	 * for exchange pricing and balancing.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Economy",
		meta = (ClampMin = "0", UIMin = "0", UIMax = "10000")
	)
	int32 BaseExchangeValue = 1;

	/*
	 * Determines whether deposits of this material are allowed to intersect
	 * the asteroid surface and appear as visible veins or pockets.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Voraxia|Ore|Generation"
	)
	bool bCanAppearOnSurface = true;

	UFUNCTION(BlueprintPure, Category = "Voraxia|Ore")
	bool IsValidOreDefinition() const
	{
		return OreTag.IsValid() && YieldPerVoxelCell > KINDA_SMALL_NUMBER;
	}
};
