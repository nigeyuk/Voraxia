// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetHydrology.h
 * @brief Deterministic Hydrology V0 terrain contribution for Generator Version 2.
 */

#pragma once

#include "CoreMinimal.h"

#include "Planet/VoraxiaPlanetDefinition.h"

/**
 * @brief Hydrology V0 terrain contribution evaluated independently of final terrain composition.
 *
 * This first pass identifies deterministic basin-prone terrain and drainage
 * corridors. It does not claim to solve global river routing, which requires a
 * later non-local flow-accumulation solver.
 */
struct FVoraxiaPlanetHydrologySample
{
	/** @brief Positive broad basin-carving depth in metres. */
	double BasinCarvingDepthMetres = 0.0;

	/** @brief Positive drainage-corridor carving depth in metres. */
	double ChannelCarvingDepthMetres = 0.0;

	/** @brief Broad deterministic basin suitability in the 0 to 1 range. */
	double BasinPotential = 0.0;

	/** @brief Deterministic drainage-corridor suitability in the 0 to 1 range. */
	double DrainagePotential = 0.0;

	/** @brief Combined Hydrology V0 diagnostic signal in the 0 to 1 range. */
	double Hydrology = 0.0;
};

/** @brief Samples deterministic Hydrology V0 contributions from resolved controls. */
namespace VoraxiaPlanetHydrology
{
	/**
	 * @brief Samples broad basin and drainage-corridor terrain contributions.
	 *
	 * Hydrology.Intensity controls carving strength. Hydrology.Frequency controls
	 * drainage-province scale and density rather than generating literal rivers.
	 *
	 * @param RuntimeState Valid immutable planet generation contract.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param Continentalness Broad base-terrain continental signal.
	 * @param OutSample Receives deterministic Hydrology V0 contribution.
	 *
	 * @return True when the contribution was calculated successfully.
	 */
	VORAXIAPLANET_API bool SampleHydrology(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		double Continentalness,
		FVoraxiaPlanetHydrologySample& OutSample);
}
