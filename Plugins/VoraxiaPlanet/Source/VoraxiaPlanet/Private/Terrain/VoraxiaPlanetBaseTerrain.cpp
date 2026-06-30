// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetBaseTerrain.cpp
 * @brief Implementation of Generator Version 2 quiet-world base terrain.
 */

#include "Terrain/VoraxiaPlanetBaseTerrain.h"

#include "Terrain/VoraxiaPlanetTerrainNoise.h"

bool VoraxiaPlanetBaseTerrain::SampleBaseTerrain(
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const FVector3d& UnitDirection,
	FVoraxiaPlanetBaseTerrainSample& OutSample)
{
	OutSample = FVoraxiaPlanetBaseTerrainSample();

	if (!RuntimeState.IsValid()
		|| UnitDirection.SizeSquared() <= 0.000000000001)
	{
		return false;
	}

	const FVector3d SafeUnitDirection =
		UnitDirection.GetSafeNormal();

	const uint32 Seed =
		static_cast<uint32>(RuntimeState.Seed);

	/**
	 * The continental field remains broad and continuous across every cube-face
	 * seam. It defines where worlds tend towards basin, lowland, plateau, or
	 * elevated continental land before more specific feature modules contribute.
	 */
	const double Continentalness =
		VoraxiaPlanetTerrainNoise::SampleFractalValueNoise3D(
			SafeUnitDirection,
			Seed ^ 0xA511E9B3u,
			1.20,
			4,
			2.05,
			0.50);

	const double LandMask =
		VoraxiaPlanetTerrainNoise::SmoothRange(
			-0.18,
			0.36,
			Continentalness);

	const double Highlandness =
		VoraxiaPlanetTerrainNoise::SmoothRange(
			0.12,
			0.72,
			Continentalness);

	const double BasinMask =
		1.0 - VoraxiaPlanetTerrainNoise::SmoothRange(
			-0.88,
			-0.10,
			Continentalness);

	const double SafeMaximumTerrainHeightMetres =
		FMath::Max(
			0.0,
			RuntimeState.MaximumTerrainHeightMetres);

	const double SafeMaximumTerrainDepthMetres =
		FMath::Max(
			0.0,
			RuntimeState.MaximumTerrainDepthMetres);

	/**
	 * Base terrain deliberately remains modest. The prominent mountain belts,
	 * rifts, impacts, and volcanic regions of Generator Version 2 are supplied
	 * by independent optional modules.
	 */
	const double PlateauHeightMetres =
		SafeMaximumTerrainHeightMetres
		* 0.14
		* LandMask;

	const double HighlandHeightMetres =
		SafeMaximumTerrainHeightMetres
		* 0.08
		* Highlandness;

	const double BasinDepthMetres =
		SafeMaximumTerrainDepthMetres
		* BasinMask;

	OutSample.HeightMetres =
		PlateauHeightMetres
		+ HighlandHeightMetres
		- BasinDepthMetres;

	OutSample.Continentalness = Continentalness;
	OutSample.Highlandness = Highlandness;

	return FMath::IsFinite(OutSample.HeightMetres)
		&& FMath::IsFinite(OutSample.Continentalness)
		&& FMath::IsFinite(OutSample.Highlandness);
}
