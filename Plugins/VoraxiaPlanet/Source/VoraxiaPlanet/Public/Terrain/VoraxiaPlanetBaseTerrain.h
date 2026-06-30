// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetBaseTerrain.h
 * @brief Deterministic quiet-world macro terrain foundation for Generator Version 2.
 */

#pragma once

#include "CoreMinimal.h"

#include "Planet/VoraxiaPlanetDefinition.h"

/**
 * @brief Baseline macro-terrain result prior to optional geological feature modules.
 *
 * This is deliberately a low-complexity, tectonically quiet foundation. Feature
 * modules such as tectonics, impacts, volcanism, hydrology, and ice modify this
 * result in the central terrain generator instead of embedding their behaviour
 * into the base-landform implementation.
 */
struct FVoraxiaPlanetBaseTerrainSample
{
	/**
	 * @brief Baseline height relative to the mathematical reference sphere.
	 */
	double HeightMetres = 0.0;

	/**
	 * @brief Broad continent-to-basin field in the approximate -1 to +1 range.
	 */
	double Continentalness = 0.0;

	/**
	 * @brief Baseline upland emphasis in the 0 to 1 range.
	 */
	double Highlandness = 0.0;
};

/**
 * @brief Samples the quiet base-landform contribution for Generator Version 2.
 */
namespace VoraxiaPlanetBaseTerrain
{
	/**
	 * @brief Samples broad continents, plateaus, highlands, and basins.
	 *
	 * @param RuntimeState Valid immutable planet generation contract.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param OutSample Receives the deterministic base terrain sample.
	 *
	 * @return True when the sample was produced successfully.
	 */
	VORAXIAPLANET_API bool SampleBaseTerrain(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		FVoraxiaPlanetBaseTerrainSample& OutSample);
}
