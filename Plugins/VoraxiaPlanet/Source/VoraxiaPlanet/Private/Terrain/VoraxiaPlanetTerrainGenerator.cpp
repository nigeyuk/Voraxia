// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetTerrainGenerator.cpp
 * @brief Central deterministic macro-terrain dispatch and feature composition.
 */

#include "Terrain/VoraxiaPlanetTerrainGenerator.h"

#include "Terrain/Features/VoraxiaPlanetTectonics.h"
#include "Terrain/VoraxiaPlanetBaseTerrain.h"
#include "Terrain/VoraxiaPlanetTerrainNoise.h"

namespace
{
	/**
	 * @brief Original persistent macro-terrain algorithm.
	 *
	 * This version must remain mathematically stable for worlds created using
	 * GeneratorVersion 1. It now uses shared noise helpers only to keep generic
	 * deterministic noise implementation out of this dispatch file.
	 */
	constexpr int32 MacroTerrainGeneratorVersion1 = 1;

	/**
	 * @brief Modular feature-composition terrain algorithm.
	 *
	 * Version 2 retains a quiet base-landform module and adds independently
	 * evaluated terrain contributions. New feature families belong in their own
	 * source files and are composed here in a stable documented order.
	 */
	constexpr int32 MacroTerrainGeneratorVersion2 = 2;

	/**
	 * @brief Samples the immutable macro-terrain algorithm for Generator Version 1.
	 *
	 * @param RuntimeState Valid planet runtime state.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param OutSample Receives macro terrain values.
	 *
	 * @return True when terrain was sampled successfully.
	 */
	bool SampleMacroTerrainVersion1(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		FVoraxiaPlanetTerrainSample& OutSample)
	{
		const uint32 Seed =
			static_cast<uint32>(RuntimeState.Seed);

		const double Continentalness =
			VoraxiaPlanetTerrainNoise::SampleFractalValueNoise3D(
				UnitDirection,
				Seed ^ 0xA511E9B3u,
				1.20,
				4,
				2.05,
				0.50);

		const double RidgeSource =
			VoraxiaPlanetTerrainNoise::SampleFractalValueNoise3D(
				UnitDirection,
				Seed ^ 0x63D83595u,
				4.75,
				4,
				2.10,
				0.52);

		const double Ridge =
			1.0 - FMath::Abs(RidgeSource);

		const double LandMask =
			VoraxiaPlanetTerrainNoise::SmoothRange(
				-0.18,
				0.36,
				Continentalness);

		const double MountainMask =
			VoraxiaPlanetTerrainNoise::SmoothRange(
				0.06,
				0.78,
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

		const double PlateauHeightMetres =
			SafeMaximumTerrainHeightMetres
			* 0.14
			* LandMask;

		const double MountainHeightMetres =
			SafeMaximumTerrainHeightMetres
			* 0.86
			* MountainMask
			* Ridge
			* Ridge;

		const double BasinDepthMetres =
			SafeMaximumTerrainDepthMetres
			* BasinMask;

		OutSample.HeightMetres =
			PlateauHeightMetres
			+ MountainHeightMetres
			- BasinDepthMetres;

		OutSample.Continentalness = Continentalness;
		OutSample.Mountainness = MountainMask * Ridge;

		return FMath::IsFinite(OutSample.HeightMetres)
			&& FMath::IsFinite(OutSample.Continentalness)
			&& FMath::IsFinite(OutSample.Mountainness);
	}

	/**
	 * @brief Samples modular macro terrain for Generator Version 2.
	 *
	 * Composition order is intentionally explicit:
	 *
	 * 1. Quiet base continents, highlands, and basins.
	 * 2. Tectonic compression uplift and extension rifts.
	 * 3. Future volcanic terrain.
	 * 4. Future impact cratering.
	 * 5. Future hydrological erosion.
	 * 6. Future ice and glacial shaping.
	 *
	 * @param RuntimeState Valid immutable planet generation contract.
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param OutSample Receives final macro terrain values.
	 *
	 * @return True when all active module samples were produced.
	 */
	bool SampleMacroTerrainVersion2(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const FVector3d& UnitDirection,
		FVoraxiaPlanetTerrainSample& OutSample)
	{
		FVoraxiaPlanetBaseTerrainSample BaseTerrain;

		if (!VoraxiaPlanetBaseTerrain::SampleBaseTerrain(
			RuntimeState,
			UnitDirection,
			BaseTerrain))
		{
			return false;
		}

		FVoraxiaPlanetTectonicsSample Tectonics;

		if (!VoraxiaPlanetTectonics::SampleTectonics(
			RuntimeState,
			UnitDirection,
			BaseTerrain.Continentalness,
			Tectonics))
		{
			return false;
		}

		const double SafeMaximumTerrainHeightMetres =
			FMath::Max(
				0.0,
				RuntimeState.MaximumTerrainHeightMetres);

		const double SafeMaximumTerrainDepthMetres =
			FMath::Max(
				0.0,
				RuntimeState.MaximumTerrainDepthMetres);

		/**
		 * Terrain limits remain an immutable generation contract. Feature modules
		 * may shape the landscape within them but must not produce out-of-bounds
		 * final surface positions that contradict the planet definition.
		 */
		OutSample.HeightMetres =
			FMath::Clamp(
				BaseTerrain.HeightMetres
				+ Tectonics.UpliftMetres
				- Tectonics.RiftDepthMetres,
				-SafeMaximumTerrainDepthMetres,
				SafeMaximumTerrainHeightMetres);

		OutSample.Continentalness =
			BaseTerrain.Continentalness;

		OutSample.Mountainness =
			FMath::Clamp(
				FMath::Max(
					BaseTerrain.Highlandness * 0.15,
					Tectonics.Mountainness),
				0.0,
				1.0);

		return FMath::IsFinite(OutSample.HeightMetres)
			&& FMath::IsFinite(OutSample.Continentalness)
			&& FMath::IsFinite(OutSample.Mountainness);
	}
}

bool VoraxiaPlanetTerrain::SampleMacroTerrain(
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const FVector3d& UnitDirection,
	FVoraxiaPlanetTerrainSample& OutSample)
{
	OutSample = FVoraxiaPlanetTerrainSample();

	if (!RuntimeState.IsValid()
		|| !FMath::IsFinite(UnitDirection.X)
		|| !FMath::IsFinite(UnitDirection.Y)
		|| !FMath::IsFinite(UnitDirection.Z)
		|| UnitDirection.SizeSquared() <= 0.000000000001)
	{
		return false;
	}

	const FVector3d SafeUnitDirection =
		UnitDirection.GetSafeNormal();

	switch (RuntimeState.GeneratorVersion)
	{
	case MacroTerrainGeneratorVersion1:
		return SampleMacroTerrainVersion1(
			RuntimeState,
			SafeUnitDirection,
			OutSample);

	case MacroTerrainGeneratorVersion2:
		return SampleMacroTerrainVersion2(
			RuntimeState,
			SafeUnitDirection,
			OutSample);

	default:
		/**
		 * Unknown generator versions fail closed. Rendering invented terrain for
		 * a planet version this build does not understand would create a visual
		 * and gameplay desync between machines.
		 */
		return false;
	}
}

bool VoraxiaPlanetTerrain::SampleMacroTerrainHeightMetres(
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const FVector3d& UnitDirection,
	double& OutHeightMetres)
{
	OutHeightMetres = 0.0;

	FVoraxiaPlanetTerrainSample Sample;

	if (!SampleMacroTerrain(
		RuntimeState,
		UnitDirection,
		Sample))
	{
		return false;
	}

	OutHeightMetres = Sample.HeightMetres;

	return true;
}
