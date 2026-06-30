// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetTectonics.cpp
 * @brief Implementation of deterministic tectonic terrain contribution sampling.
 */

#include "Terrain/Features/VoraxiaPlanetTectonics.h"

#include "Terrain/VoraxiaPlanetTerrainNoise.h"

namespace
{
	/**
	 * @brief Clamps a feature-profile control into the supported terrain range.
	 *
	 * @param Value Authored feature strength or frequency.
	 *
	 * @return Safe non-negative contribution value.
	 */
	double ClampFeatureControl(const double Value)
	{
		return FMath::Clamp(Value, 0.0, 3.0);
	}
}

bool VoraxiaPlanetTectonics::SampleTectonics(
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const FVector3d& UnitDirection,
	const double Continentalness,
	FVoraxiaPlanetTectonicsSample& OutSample)
{
	OutSample = FVoraxiaPlanetTectonicsSample();

	if (!RuntimeState.IsValid()
		|| UnitDirection.SizeSquared() <= 0.000000000001
		|| !FMath::IsFinite(Continentalness))
	{
		return false;
	}

	/**
	 * A disabled tectonics control is a valid deterministic zero contribution.
	 * This permits quiet worlds without special branches in the master generator.
	 */
	if (!RuntimeState.Tectonics.IsActive())
	{
		return true;
	}

	const FVector3d SafeUnitDirection =
		UnitDirection.GetSafeNormal();

	const double Intensity =
		ClampFeatureControl(RuntimeState.Tectonics.Intensity);

	const double Frequency =
		ClampFeatureControl(RuntimeState.Tectonics.Frequency);

	if (Intensity <= 0.0 || Frequency <= 0.0)
	{
		return true;
	}

	const double FrequencyNormalised =
		FMath::Clamp(Frequency / 1.50, 0.0, 1.0);

	/**
	 * Lower frequencies produce fewer, broader plate provinces. Higher values
	 * increase the number and spacing of provinces, not merely noisy detail.
	 */
	const double ProvinceFrequency =
		FMath::Lerp(1.15, 5.80, FrequencyNormalised);

	const double RidgeFrequency =
		FMath::Lerp(3.20, 10.50, FrequencyNormalised);

	const uint32 Seed =
		static_cast<uint32>(RuntimeState.Seed);

	const double ProvinceSignal =
		VoraxiaPlanetTerrainNoise::SampleFractalValueNoise3D(
			SafeUnitDirection,
			Seed ^ 0x4C18A72Du,
			ProvinceFrequency,
			3,
			2.05,
			0.52);

	/**
	 * Plate-boundary emphasis creates regional belts rather than applying
	 * mountains uniformly across every continental area.
	 */
	const double BoundarySignal =
		VoraxiaPlanetTerrainNoise::Clamp01(
			1.0 - FMath::Abs(ProvinceSignal));

	const double BoundaryMask =
		VoraxiaPlanetTerrainNoise::SmoothRange(
			0.42,
			0.90,
			BoundarySignal);

	const double ContinentalMountainMask =
		VoraxiaPlanetTerrainNoise::SmoothRange(
			0.04,
			0.72,
			Continentalness);

	const double Ridge =
		VoraxiaPlanetTerrainNoise::SampleRidgedFractalNoise3D(
			SafeUnitDirection,
			Seed ^ 0xA05B76E1u,
			RidgeFrequency,
			4,
			2.10,
			0.52);

	/**
	 * A separate deterministic selector decides which boundary sections lean
	 * towards compressive uplift versus extensional rifting. It is intentionally
	 * lower frequency than the ridge signal to retain regional coherence.
	 */
	const double BoundaryCharacterSource =
		VoraxiaPlanetTerrainNoise::SampleFractalValueNoise3D(
			SafeUnitDirection,
			Seed ^ 0xD16F21B9u,
			ProvinceFrequency * 0.80,
			2,
			2.00,
			0.55);

	const double ExtensionMask =
		VoraxiaPlanetTerrainNoise::SmoothRange(
			0.02,
			0.68,
			(BoundaryCharacterSource + 1.0) * 0.50);

	const double CompressionMask =
		1.0 - ExtensionMask;

	const double SafeMaximumTerrainHeightMetres =
		FMath::Max(
			0.0,
			RuntimeState.MaximumTerrainHeightMetres);

	const double SafeMaximumTerrainDepthMetres =
		FMath::Max(
			0.0,
			RuntimeState.MaximumTerrainDepthMetres);

	const double MountainBeltMask =
		BoundaryMask
		* ContinentalMountainMask
		* CompressionMask;

	const double RiftMask =
		BoundaryMask
		* ExtensionMask;

	OutSample.UpliftMetres =
		SafeMaximumTerrainHeightMetres
		* 0.76
		* Intensity
		* MountainBeltMask
		* Ridge
		* Ridge;

	OutSample.RiftDepthMetres =
		SafeMaximumTerrainDepthMetres
		* 0.58
		* Intensity
		* RiftMask
		* FMath::Lerp(0.45, 1.0, Ridge);

	OutSample.Mountainness =
		FMath::Clamp(
			MountainBeltMask * Ridge,
			0.0,
			1.0);

	OutSample.Activity =
		FMath::Clamp(
			BoundaryMask * Intensity,
			0.0,
			1.0);

	return FMath::IsFinite(OutSample.UpliftMetres)
		&& FMath::IsFinite(OutSample.RiftDepthMetres)
		&& FMath::IsFinite(OutSample.Mountainness)
		&& FMath::IsFinite(OutSample.Activity);
}
