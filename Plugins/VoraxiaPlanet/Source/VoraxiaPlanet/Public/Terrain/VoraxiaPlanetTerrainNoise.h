// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetTerrainNoise.h
 * @brief Shared deterministic direction-space noise helpers for Voraxia terrain modules.
 */

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Stateless deterministic noise utilities used by Voraxia terrain generation.
 *
 * These functions operate exclusively on supplied values. They do not access
 * UObjects, world state, networking, random streams, or editor state.
 *
 * Sampling in three-dimensional planet-direction space keeps all feature fields
 * continuous across cube-face boundaries.
 */
namespace VoraxiaPlanetTerrainNoise
{
	/**
	 * @brief Clamps a scalar to the inclusive 0 to 1 range.
	 *
	 * @param Value Source scalar.
	 *
	 * @return Clamped scalar.
	 */
	VORAXIAPLANET_API double Clamp01(double Value);

	/**
	 * @brief Applies smooth cubic interpolation to a 0 to 1 scalar.
	 *
	 * @param Value Source interpolation scalar.
	 *
	 * @return Cubic smooth-step interpolation scalar.
	 */
	VORAXIAPLANET_API double SmoothStep01(double Value);

	/**
	 * @brief Maps a source scalar through a smooth inclusive range.
	 *
	 * @param Minimum Lower source bound.
	 * @param Maximum Upper source bound.
	 * @param Value Source scalar.
	 *
	 * @return Smooth 0 to 1 range result.
	 */
	VORAXIAPLANET_API double SmoothRange(
		double Minimum,
		double Maximum,
		double Value);

	/**
	 * @brief Samples coherent signed deterministic value noise.
	 *
	 * @param Position Direction-space sample position.
	 * @param Seed Deterministic seed for this field.
	 *
	 * @return Coherent signed value approximately in the -1 to +1 range.
	 */
	VORAXIAPLANET_API double SampleValueNoise3D(
		const FVector3d& Position,
		uint32 Seed);

	/**
	 * @brief Samples layered coherent signed deterministic value noise.
	 *
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param Seed Deterministic base seed.
	 * @param BaseFrequency Initial direction-space frequency.
	 * @param Octaves Number of layered samples.
	 * @param Lacunarity Frequency multiplier per octave.
	 * @param Gain Amplitude multiplier per octave.
	 *
	 * @return Normalised signed fractal value approximately in the -1 to +1 range.
	 */
	VORAXIAPLANET_API double SampleFractalValueNoise3D(
		const FVector3d& UnitDirection,
		uint32 Seed,
		double BaseFrequency,
		int32 Octaves,
		double Lacunarity,
		double Gain);

	/**
	 * @brief Samples a 0 to 1 ridged fractal field.
	 *
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param Seed Deterministic base seed.
	 * @param BaseFrequency Initial direction-space frequency.
	 * @param Octaves Number of layered samples.
	 * @param Lacunarity Frequency multiplier per octave.
	 * @param Gain Amplitude multiplier per octave.
	 *
	 * @return Ridged field where higher values favour narrow crests.
	 */
	VORAXIAPLANET_API double SampleRidgedFractalNoise3D(
		const FVector3d& UnitDirection,
		uint32 Seed,
		double BaseFrequency,
		int32 Octaves,
		double Lacunarity,
		double Gain);
}
