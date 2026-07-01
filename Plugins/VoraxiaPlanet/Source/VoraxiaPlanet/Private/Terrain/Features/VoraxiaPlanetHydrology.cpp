// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetHydrology.cpp
 * @brief Implementation of deterministic Hydrology V0 terrain contribution sampling.
 */

#include "Terrain/Features/VoraxiaPlanetHydrology.h"

#include "Terrain/VoraxiaPlanetTerrainNoise.h"

namespace
{
	/**
	 * @brief Clamps an authored hydrology control into the supported terrain range.
	 *
	 * @param Value Authored feature strength or frequency.
	 *
	 * @return Safe non-negative contribution value.
	 */
	double ClampHydrologyFeatureControl(const double Value)
	{
		return FMath::Clamp(Value, 0.0, 3.0);
	}
}

bool VoraxiaPlanetHydrology::SampleHydrology(
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const FVector3d& UnitDirection,
	const double Continentalness,
	FVoraxiaPlanetHydrologySample& OutSample)
{
	OutSample = FVoraxiaPlanetHydrologySample();

	if (!RuntimeState.IsValid()
		|| UnitDirection.SizeSquared() <= 0.000000000001
		|| !FMath::IsFinite(Continentalness))
	{
		return false;
	}

	if (!RuntimeState.Hydrology.IsActive())
	{
		return true;
	}

	const FVector3d SafeUnitDirection = UnitDirection.GetSafeNormal();
	const double Intensity = ClampHydrologyFeatureControl(RuntimeState.Hydrology.Intensity);
	const double Frequency = ClampHydrologyFeatureControl(RuntimeState.Hydrology.Frequency);

	if (Intensity <= 0.0 || Frequency <= 0.0)
	{
		return true;
	}

	const double FrequencyNormalised = FMath::Clamp(Frequency / 1.50, 0.0, 1.0);
	const double BasinProvinceFrequency = FMath::Lerp(1.35, 6.20, FrequencyNormalised);
	const double DrainageCorridorFrequency = FMath::Lerp(3.40, 13.20, FrequencyNormalised);
	const uint32 Seed = static_cast<uint32>(RuntimeState.Seed);

	const double BasinProvinceSignal = VoraxiaPlanetTerrainNoise::SampleFractalValueNoise3D(
		SafeUnitDirection,
		Seed ^ 0x2E6D4A91u,
		BasinProvinceFrequency,
		3,
		2.05,
		0.52);

	const double LowlandMask = 1.0 - VoraxiaPlanetTerrainNoise::SmoothRange(
		-0.08,
		0.58,
		Continentalness);

	const double BasinPotential = VoraxiaPlanetTerrainNoise::SmoothRange(
		-0.70,
		0.48,
		-BasinProvinceSignal)
		* FMath::Lerp(0.45, 1.0, LowlandMask);

	const double DrainageRidge = VoraxiaPlanetTerrainNoise::SampleRidgedFractalNoise3D(
		SafeUnitDirection,
		Seed ^ 0x75B84CF2u,
		DrainageCorridorFrequency,
		3,
		2.05,
		0.50);

	const double CorridorMask = VoraxiaPlanetTerrainNoise::SmoothRange(
		0.62,
		0.93,
		DrainageRidge);

	const double LandDrainageMask = VoraxiaPlanetTerrainNoise::SmoothRange(
		-0.20,
		0.50,
		Continentalness);

	const double BasinSuppression = 1.0 - FMath::Clamp(BasinPotential * 0.65, 0.0, 0.65);
	const double DrainagePotential = CorridorMask * LandDrainageMask * BasinSuppression;
	const double SafeMaximumTerrainDepthMetres = FMath::Max(0.0, RuntimeState.MaximumTerrainDepthMetres);

	/**
	 * Hydrology V0 remains deliberately subtle. It contributes broad erosion
	 * tendency and later PCG/biome hooks without inventing deep local river cuts.
	 */
	OutSample.BasinCarvingDepthMetres = SafeMaximumTerrainDepthMetres * 0.22 * Intensity * BasinPotential;
	OutSample.ChannelCarvingDepthMetres = SafeMaximumTerrainDepthMetres * 0.085 * Intensity * DrainagePotential;
	OutSample.BasinPotential = FMath::Clamp(BasinPotential, 0.0, 1.0);
	OutSample.DrainagePotential = FMath::Clamp(DrainagePotential, 0.0, 1.0);
	OutSample.Hydrology = FMath::Clamp(FMath::Max(OutSample.BasinPotential, OutSample.DrainagePotential * 0.80), 0.0, 1.0);

	return FMath::IsFinite(OutSample.BasinCarvingDepthMetres)
		&& FMath::IsFinite(OutSample.ChannelCarvingDepthMetres)
		&& FMath::IsFinite(OutSample.BasinPotential)
		&& FMath::IsFinite(OutSample.DrainagePotential)
		&& FMath::IsFinite(OutSample.Hydrology);
}
