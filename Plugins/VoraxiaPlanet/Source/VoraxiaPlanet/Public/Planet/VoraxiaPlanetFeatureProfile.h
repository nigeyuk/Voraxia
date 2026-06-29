// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetFeatureProfile.h
 * @brief Data-driven authored feature profile for Voraxia planetary generation.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/DataAsset.h"

#include "VoraxiaPlanetFeatureProfile.generated.h"

/**
 * @brief Describes the broad class of celestial body represented by a profile.
 *
 * This is intentionally a generation and validation category rather than a
 * visual label. Future systems use it to determine which geological,
 * atmospheric, and PCG feature modules are sensible for the body.
 */
UENUM(BlueprintType)
enum class EVoraxiaCelestialBodyType : uint8
{
	/** @brief A full-sized rocky or icy planet orbiting a star. */
	Planet UMETA(DisplayName = "Planet"),

	/** @brief A natural satellite orbiting another celestial body. */
	Moon UMETA(DisplayName = "Moon"),

	/** @brief A smaller planetary body with limited geological scale. */
	DwarfPlanet UMETA(DisplayName = "Dwarf Planet"),

	/** @brief A small rocky, metallic, or icy body. */
	Asteroid UMETA(DisplayName = "Asteroid")
};

/**
 * @brief Controls the strength of one optional planetary generation process.
 *
 * A feature is not represented by a bare boolean because most generation
 * processes require an authored scale and distribution rather than a simple
 * on/off switch.
 */
USTRUCT(BlueprintType)
struct VORAXIAPLANET_API FVoraxiaPlanetFeatureControl
{
	GENERATED_BODY()

	/**
	 * @brief Enables this feature module for planets using the profile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature")
	bool bEnabled = false;

	/**
	 * @brief Relative authored influence of this feature.
	 *
	 * A value of 0 means visually or mechanically negligible influence, while
	 * 1 represents the normal intended profile strength. Values above 1 are
	 * allowed for deliberately extreme worlds.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Feature",
		meta = (
			ClampMin = "0.0",
			ClampMax = "3.0",
			UIMin = "0.0",
			UIMax = "1.5"))
	double Intensity = 1.0;

	/**
	 * @brief Relative frequency or spatial coverage of this feature.
	 *
	 * This will later control such things as crater density, volcanic hotspot
	 * count, cave-region frequency, or ice coverage.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Feature",
		meta = (
			ClampMin = "0.0",
			ClampMax = "3.0",
			UIMin = "0.0",
			UIMax = "1.5"))
	double Frequency = 1.0;

	/**
	 * @brief Returns whether the feature should contribute to generation.
	 *
	 * @return True when enabled and carrying meaningful authored influence.
	 */
	bool IsActive() const
	{
		return bEnabled
			&& Intensity > 0.0
			&& Frequency > 0.0;
	}
};

/**
 * @brief Data asset describing which world-generation processes a planet may use.
 *
 * This asset is authored content. It does not itself represent a persistent
 * planet identity, runtime terrain state, or player-edit state.
 *
 * A planet definition will later reference a specific feature profile and lock
 * the resolved profile identifier/version into its generation contract.
 *
 * @warning Existing planets must not silently change shape because an assigned
 * profile is edited later. Planet creation will eventually capture a resolved
 * immutable feature contract in replicated runtime state.
 */
UCLASS(BlueprintType)
class VORAXIAPLANET_API UVoraxiaPlanetFeatureProfile
	: public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * @brief Stable Asset Manager type for planet feature profile assets.
	 */
	static const FPrimaryAssetType PrimaryAssetType;

	/**
	 * @brief Human-readable profile title for designers and documentation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText ProfileDisplayName;

	/**
	 * @brief Immutable authored revision of this profile's feature rules.
	 *
	 * Increase this only when the profile's generation behaviour changes in a
	 * way that must be distinguishable from older worlds.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Identity",
		meta = (ClampMin = "1"))
	int32 ProfileVersion = 1;

	/**
	 * @brief Broad celestial-body classification used by future validation rules.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial Context")
	EVoraxiaCelestialBodyType BodyType =
		EVoraxiaCelestialBodyType::Planet;

	/**
	 * @brief Whether this body is a moon of another planetary body.
	 *
	 * This remains explicit even though BodyType may also be Moon, because
	 * future generated bodies can use unusual classifications while still being
	 * gravitationally or tidally associated with a parent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial Context")
	bool bHasParentBody = false;

	/**
	 * @brief Relative influence of impact craters and impact basins.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Features")
	FVoraxiaPlanetFeatureControl ImpactCratering;

	/**
	 * @brief Relative influence of large-scale tectonic fractures and mountain belts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Features")
	FVoraxiaPlanetFeatureControl Tectonics;

	/**
	 * @brief Relative influence of volcanic or cryovolcanic terrain processes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Features")
	FVoraxiaPlanetFeatureControl Volcanism;

	/**
	 * @brief Relative influence of surface liquid flow, drainage, and basin erosion.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Features")
	FVoraxiaPlanetFeatureControl Hydrology;

	/**
	 * @brief Relative influence of polar, glacial, or global ice processes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Features")
	FVoraxiaPlanetFeatureControl Ice;

	/**
	 * @brief Relative influence of caves, lava tubes, fracture systems, and voids.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subsurface Features")
	FVoraxiaPlanetFeatureControl Caves;

	/**
	 * @brief Relative influence of geological ore bodies and resource belts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subsurface Features")
	FVoraxiaPlanetFeatureControl OreBodies;

	/**
	 * @brief Relative influence of unusual regional hazards and hostile terrain.
	 *
	 * Future examples include radiation belts, corrosive zones, unstable ground,
	 * toxic basins, geothermal vents, and electrical storm regions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environmental Features")
	FVoraxiaPlanetFeatureControl Hazards;

	/**
	 * @brief Returns this asset's stable primary asset identifier.
	 *
	 * @return Asset Manager identifier for this feature profile.
	 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * @brief Validates the authored profile for safe use by planet generation.
	 *
	 * @param OutFailureReason Receives a human-readable validation failure.
	 *
	 * @return True when the profile is suitable for use.
	 */
	bool IsProfileValid(FString& OutFailureReason) const;
};
