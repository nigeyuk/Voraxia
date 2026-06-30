// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetImpactCratering.h
 * @brief Deterministic impact-crater terrain contribution for Generator Version 2.
 */

#pragma once

#include "CoreMinimal.h"

#include "Planet/VoraxiaPlanetDefinition.h"

/**
 * @brief Impact-crater terrain contribution evaluated independently of final terrain composition.
 *
 * The contribution represents a single strongest local impact feature sampled
 * from the deterministic global crater field. It never owns terrain state and
 * is composed by the central terrain generator.
 */
struct FVoraxiaPlanetImpactCrateringSample
{
	/**
	 * @brief Positive basin-depth magnitude in metres.
	 *
	 * The central terrain generator subtracts this value from final height.
	 */
	double BasinDepthMetres = 0.0;

	/**
	 * @brief Positive raised-rim height in metres.
	 */
	double RimHeightMetres = 0.0;

	/**
	 * @brief Positive outer ejecta and disturbed-ground height in metres.
	 */
	double EjectaHeightMetres = 0.0;

	/**
	 * @brief Local impact-landform emphasis in the 0 to 1 range.
	 *
	 * This is intended for later debug colouring, biome classification,
	 * material variation, and gameplay-region inspection.
	 */
	double Craterness = 0.0;
};

/**
 * @brief Samples deterministic impact-crater terrain contributions from resolved controls.
 */
namespace VoraxiaPlanetImpactCratering
{
	/**
	 * @brief Samples the local basin, rim, and ejecta contribution of the crater field.
	 *
	 * ImpactCratering.Intensity controls crater depth, rim relief, and ejecta
	 * influence. ImpactCratering.Frequency controls candidate density and typical
	 * spacing, rather than adding arbitrary high-frequency surface noise.
	 *
	 * @param RuntimeState Valid immutable planet generation contract.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param OutSample Receives deterministic crater terrain contribution.
	 *
	 * @return True when the contribution was calculated successfully.
	 */
	VORAXIAPLANET_API bool SampleImpactCratering(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		FVoraxiaPlanetImpactCrateringSample& OutSample);
}
