// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetTectonics.h
 * @brief Deterministic tectonic mountain-belt and rift contribution for Generator Version 2.
 */

#pragma once

#include "CoreMinimal.h"

#include "Planet/VoraxiaPlanetDefinition.h"

/**
 * @brief Tectonic terrain contribution evaluated independently of final terrain composition.
 *
 * This contribution never writes terrain state. It is calculated locally from
 * immutable runtime data and combined by the central macro terrain generator.
 */
struct FVoraxiaPlanetTectonicsSample
{
	/**
	 * @brief Positive mountain-belt uplift in metres.
	 */
	double UpliftMetres = 0.0;

	/**
	 * @brief Positive rift-depth magnitude in metres.
	 *
	 * The central terrain generator subtracts this value from final height.
	 */
	double RiftDepthMetres = 0.0;

	/**
	 * @brief Tectonic mountain emphasis in the 0 to 1 range.
	 */
	double Mountainness = 0.0;

	/**
	 * @brief General tectonic activity signal in the 0 to 1 range.
	 */
	double Activity = 0.0;
};

/**
 * @brief Samples tectonic terrain contributions from resolved feature-profile controls.
 */
namespace VoraxiaPlanetTectonics
{
	/**
	 * @brief Samples deterministic mountain-belt uplift and rift depression.
	 *
	 * Tectonics.Intensity controls the magnitude of uplift and rifting.
	 * Tectonics.Frequency controls the number and spacing of broad tectonic
	 * provinces rather than merely adding higher-frequency surface noise.
	 *
	 * @param RuntimeState Valid immutable planet generation contract.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param Continentalness Base-terrain continental field at this location.
	 * @param OutSample Receives tectonic terrain contribution.
	 *
	 * @return True when the contribution was calculated successfully.
	 */
	VORAXIAPLANET_API bool SampleTectonics(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		double Continentalness,
		FVoraxiaPlanetTectonicsSample& OutSample);
}
