// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetVolcanism.h
 * @brief Deterministic volcanic terrain contribution for Generator Version 2.
 */

#pragma once

#include "CoreMinimal.h"
#include "Planet/VoraxiaPlanetDefinition.h"

/** @brief Volcanic terrain contribution evaluated independently of final composition. */
struct FVoraxiaPlanetVolcanismSample
{
	/** @brief Positive broad shield uplift in metres. */
	double ShieldUpliftMetres = 0.0;

	/** @brief Positive central caldera-depth magnitude in metres. */
	double CalderaDepthMetres = 0.0;

	/** @brief Positive low-relief lava-plain contribution in metres. */
	double LavaPlainHeightMetres = 0.0;

	/** @brief Local volcanic landform emphasis in the 0 to 1 range. */
	double Volcanism = 0.0;
};

/** @brief Samples deterministic volcanic terrain contributions from resolved controls. */
namespace VoraxiaPlanetVolcanism
{
	/**
	 * @brief Samples local shield, caldera, and lava-plain contribution.
	 *
	 * Volcanism.Intensity controls shield uplift, caldera depth, and lava-plain
	 * strength. Volcanism.Frequency controls volcanic-system density and spacing.
	 *
	 * @param RuntimeState Valid immutable planet generation contract.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param OutSample Receives deterministic volcanic terrain contribution.
	 * @return True when the contribution was calculated successfully.
	 */
	VORAXIAPLANET_API bool SampleVolcanism(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		FVoraxiaPlanetVolcanismSample& OutSample);
}
