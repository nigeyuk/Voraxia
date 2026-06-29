// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/DataAsset.h"

#include "Core/VoraxiaPlanetTypes.h"
#include "Planet/VoraxiaPlanetFeatureProfile.h"

#include "VoraxiaPlanetDefinition.generated.h"

/**
 * The compact, authoritative planet information replicated by AVoraxiaPlanetActor.
 *
 * This is intentionally data-only. Later systems will use it to initialise
 * deterministic generation, terrain streaming, gravity, networking, and saves.
 */
USTRUCT(BlueprintType)
struct VORAXIAPLANET_API FVoraxiaPlanetRuntimeState
{
	GENERATED_BODY()

public:
	/**
	 * Permanent identity for this planet.
	 *
	 * This must never change once terrain edits, structures, or player data
	 * exist for the planet.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	FVoraxiaPlanetId PlanetId;

	/**
	 * Friendly identifier for logging and diagnostics.
	 *
	 * This is the data asset's object name, not the display name shown to players.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	FName DefinitionName = NAME_None;

	/**
	 * Deterministic seed used by all machines to generate the untouched planet.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation")
	int32 Seed = 0;

	/**
	 * Version number for the generator logic.
	 *
	 * If generation mathematics changes incompatibly in future, this allows
	 * old planets to remain interpretable instead of reshaping overnight.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation")
	int32 GeneratorVersion = 1;

	/**
	 * Radius of the planet's reference sphere, expressed in metres.
	 *
	 * This is not an Unreal-world-centimetre distance.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet", meta = (Units = "m"))
	double RadiusMetres = 0.0;

	/**
	 * Maximum generated terrain height above the reference sphere.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (Units = "m"))
	double MaximumTerrainHeightMetres = 0.0;

	/**
	 * Maximum editable/generated terrain depth below the reference sphere.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (Units = "m"))
	double MaximumTerrainDepthMetres = 0.0;

	/**
	 * Surface gravity in metres per second squared.
	 *
	 * The first gravity implementation will use this as its baseline value.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
	double SurfaceGravityMetresPerSecondSquared = 0.0;

	/**
	 * Stable identifier of the authored feature profile resolved by the server.
	 *
	 * Clients generate from the copied controls below rather than dereferencing
	 * a mutable data asset while reconstructing planet content.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation")
	FPrimaryAssetId FeatureProfileId;

	/**
	 * Authored revision of the resolved feature profile.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation")
	int32 FeatureProfileVersion = 0;

	/** @brief Server-resolved impact-cratering control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Features")
	FVoraxiaPlanetFeatureControl ImpactCratering;

	/** @brief Server-resolved tectonic control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Features")
	FVoraxiaPlanetFeatureControl Tectonics;

	/** @brief Server-resolved volcanic or cryovolcanic control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Features")
	FVoraxiaPlanetFeatureControl Volcanism;

	/** @brief Server-resolved hydrology and basin-erosion control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Features")
	FVoraxiaPlanetFeatureControl Hydrology;

	/** @brief Server-resolved polar, glacial, or global ice control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Features")
	FVoraxiaPlanetFeatureControl Ice;

	/** @brief Server-resolved cave and void-generation control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Features")
	FVoraxiaPlanetFeatureControl Caves;

	/** @brief Server-resolved ore-body and geological-resource control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Features")
	FVoraxiaPlanetFeatureControl OreBodies;

	/** @brief Server-resolved environmental-hazard control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Features")
	FVoraxiaPlanetFeatureControl Hazards;

	bool IsValid() const
	{
		const auto IsFeatureControlValid =
			[](const FVoraxiaPlanetFeatureControl& FeatureControl)
			{
				return FMath::IsFinite(FeatureControl.Intensity)
					&& FMath::IsFinite(FeatureControl.Frequency)
					&& FeatureControl.Intensity >= 0.0
					&& FeatureControl.Frequency >= 0.0;
			};

		return PlanetId.IsValid()
			&& !DefinitionName.IsNone()
			&& GeneratorVersion > 0
			&& RadiusMetres > 0.0
			&& MaximumTerrainHeightMetres >= 0.0
			&& MaximumTerrainDepthMetres >= 0.0
			&& SurfaceGravityMetresPerSecondSquared > 0.0
			&& FeatureProfileId.IsValid()
			&& FeatureProfileVersion > 0
			&& FMath::IsFinite(RadiusMetres)
			&& FMath::IsFinite(MaximumTerrainHeightMetres)
			&& FMath::IsFinite(MaximumTerrainDepthMetres)
			&& FMath::IsFinite(SurfaceGravityMetresPerSecondSquared)
			&& IsFeatureControlValid(ImpactCratering)
			&& IsFeatureControlValid(Tectonics)
			&& IsFeatureControlValid(Volcanism)
			&& IsFeatureControlValid(Hydrology)
			&& IsFeatureControlValid(Ice)
			&& IsFeatureControlValid(Caves)
			&& IsFeatureControlValid(OreBodies)
			&& IsFeatureControlValid(Hazards);
	}
};

/**
 * Persistent planet configuration asset.
 *
 * One of these assets represents the durable design identity of a planet:
 * its seed, scale, generation version, and physical baseline.
 *
 * Terrain edits, mining scars, structures, ownership, and runtime chunk state
 * do NOT belong here. They will be server-owned persistent world data later.
 */
UCLASS(BlueprintType)
class VORAXIAPLANET_API UVoraxiaPlanetDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Stable Asset Manager type used by all planet definition assets.
	 */
	static const FPrimaryAssetType PrimaryAssetType;

	/**
	 * Human-readable planet name.
	 *
	 * This may be changed later without breaking saves or networking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText PlanetDisplayName;

	/**
	 * Permanent unique planet identity.
	 *
	 * Generate this once when creating a new planet and never regenerate it
	 * after the planet has entered a real save or multiplayer world.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Identity")
	FVoraxiaPlanetId PlanetId;

	/**
	 * The deterministic base-generation seed.
	 *
	 * Two machines with the same definition and generator version must create
	 * identical untouched terrain from this value.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	int32 Seed = 1337;

	/**
	 * The version of Voraxia's planet-generation rules used by this definition.
	 *
	 * Start at 1. Increase only when an incompatible generation change is made.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = "1"))
	int32 GeneratorVersion = 1;

	/**
	 * Authored feature profile used to resolve this planet's geological and
	 * environmental-generation rules.
	 *
	 * The server validates this asset, then copies its identifier, version, and
	 * feature controls into FVoraxiaPlanetRuntimeState.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	TObjectPtr<UVoraxiaPlanetFeatureProfile> FeatureProfile;

	/**
	 * Radius of the reference sphere, in metres.
	 *
	 * 6,000,000 m is approximately a 6,000 km-radius terrestrial planet.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet", meta = (ClampMin = "1.0", Units = "m"))
	double RadiusMetres = 6000000.0;

	/**
	 * Maximum terrain elevation above the reference sphere, in metres.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (ClampMin = "0.0", Units = "m"))
	double MaximumTerrainHeightMetres = 12000.0;

	/**
	 * Maximum editable terrain depth below the reference sphere, in metres.
	 *
	 * This eventually defines the initial practical depth for tunnels, caves,
	 * ore bodies, and other near-surface voxel terrain.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (ClampMin = "0.0", Units = "m"))
	double MaximumTerrainDepthMetres = 3000.0;

	/**
	 * Surface gravity in metres per second squared.
	 *
	 * Earth's approximate value is 9.81 m/s².
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics", meta = (ClampMin = "0.01"))
	double SurfaceGravityMetresPerSecondSquared = 9.81;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Creates a new permanent GUID for a brand-new planet definition.
	 *
	 * Never use this on a planet that already has terrain edits, structures,
	 * save data, or a live multiplayer identity.
	 */
	UFUNCTION(CallInEditor, Category = "Identity")
	void GenerateNewPlanetId();

	/**
	 * Verifies the definition is safe to use on a server.
	 */
	bool IsDefinitionValid(FString& OutFailureReason) const;

	/**
	 * Creates the compact server-authoritative state replicated by the planet actor.
	 */
	FVoraxiaPlanetRuntimeState CreateRuntimeState() const;
};
