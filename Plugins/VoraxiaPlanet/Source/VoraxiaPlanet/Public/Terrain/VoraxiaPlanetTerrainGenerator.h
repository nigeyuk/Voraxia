// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetTerrainGenerator.h
 * @brief Deterministic macro-terrain sampling helpers for Voraxia planets.
 */

#pragma once

#include "CoreMinimal.h"

#include "Planet/VoraxiaPlanetDefinition.h"

/**
 * @brief Result of evaluating the current macro-terrain generator at one point.
 *
 * The values are generated entirely from the immutable planet runtime state and
 * a normalised direction from the planet centre. They are safe to recreate on
 * servers and clients without replicating terrain vertex data.
 */
struct FVoraxiaPlanetTerrainSample
{
	/**
	 * @brief Terrain elevation relative to the mathematical reference sphere.
	 *
	 * Positive values represent terrain above the reference radius. Negative
	 * values represent basins below it.
	 */
	double HeightMetres = 0.0;

	/**
	 * @brief Broad continental signal in the range approximately -1 to +1.
	 *
	 * Negative values tend towards low basins. Positive values tend towards
	 * continents and elevated land.
	 */
	double Continentalness = 0.0;

	/**
	 * @brief Mountain emphasis in the range 0 to 1.
	 *
	 * This is primarily a diagnostic value for future debug colouring,
	 * biome classification, and terrain inspection tools.
	 */
	double Mountainness = 0.0;

	/**
	 * @brief Impact-landform emphasis in the range 0 to 1.
	 *
	 * Values near one indicate a crater basin, raised rim, or immediate ejecta
	 * field. It is a diagnostic signal and does not itself alter terrain height.
	 */
	double ImpactCraterness = 0.0;
};

/**
 * @brief Pure deterministic macro-terrain generation utilities.
 *
 * These functions deliberately have no UObject ownership, mesh dependency,
 * networking code, world access, or editor state. Given the same runtime state
 * and unit direction, every machine must calculate the same terrain sample.
 *
 * Generator Version 1 preserves the original monolithic macro terrain recipe.
 * Generator Version 2 composes quiet base terrain with optional independent
 * terrain feature modules, currently tectonics and impact cratering.
 */
namespace VoraxiaPlanetTerrain
{
	/**
	 * @brief Samples deterministic macro terrain at a direction from the planet centre.
	 *
	 * @param RuntimeState Valid replicated planet runtime state.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param OutSample Receives height and diagnostic macro-terrain signals.
	 *
	 * @return True when the generator version is supported and a valid result
	 * was produced.
	 *
	 * @warning GeneratorVersion is a persistent and networked terrain contract.
	 * Never alter an existing version's algorithm after worlds have been created.
	 * Add a new generator-version branch instead.
	 */
	VORAXIAPLANET_API bool SampleMacroTerrain(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		FVoraxiaPlanetTerrainSample& OutSample);

	/**
	 * @brief Samples only the deterministic terrain elevation at a planet direction.
	 *
	 * @param RuntimeState Valid replicated planet runtime state.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param OutHeightMetres Receives terrain elevation relative to the reference sphere.
	 *
	 * @return True when a valid terrain height was produced.
	 */
	VORAXIAPLANET_API bool SampleMacroTerrainHeightMetres(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		double& OutHeightMetres);
}
