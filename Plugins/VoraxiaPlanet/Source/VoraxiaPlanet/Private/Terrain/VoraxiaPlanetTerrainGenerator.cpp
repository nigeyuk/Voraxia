// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetTerrainGenerator.cpp
 * @brief Implementation of deterministic Voraxia macro-terrain sampling.
 */

#include "Terrain/VoraxiaPlanetTerrainGenerator.h"

#include <cmath>

namespace
{
	/**
	 * @brief First supported persistent macro-terrain generator version.
	 *
	 * Existing terrain generator versions must remain immutable once worlds using
	 * them can exist. New behaviour belongs in a new version branch.
	 */
	constexpr int32 MacroTerrainGeneratorVersion1 = 1;

	/**
	 * @brief Clamps a scalar to the inclusive range 0 to 1.
	 *
	 * @param Value Value to clamp.
	 *
	 * @return Clamped scalar.
	 */
	double Clamp01(const double Value)
	{
		return FMath::Clamp(Value, 0.0, 1.0);
	}

	/**
	 * @brief Smooth cubic interpolation suitable for coherent value noise.
	 *
	 * @param Value Interpolation scalar expected in the range 0 to 1.
	 *
	 * @return Smoothly eased interpolation scalar.
	 */
	double SmoothStep01(const double Value)
	{
		const double ClampedValue = Clamp01(Value);

		return ClampedValue
			* ClampedValue
			* (3.0 - (2.0 * ClampedValue));
	}

	/**
	 * @brief Maps a scalar from one range into a smooth 0 to 1 transition.
	 *
	 * @param Minimum Lower bound of the source range.
	 * @param Maximum Upper bound of the source range.
	 * @param Value Source value to remap.
	 *
	 * @return Smooth transition scalar in the range 0 to 1.
	 */
	double SmoothRange(
		const double Minimum,
		const double Maximum,
		const double Value)
	{
		if (Maximum <= Minimum)
		{
			return 0.0;
		}

		return SmoothStep01(
			(Value - Minimum)
			/ (Maximum - Minimum));
	}

	/**
	 * @brief Linearly interpolates between two scalar values.
	 *
	 * @param A First value.
	 * @param B Second value.
	 * @param Alpha Interpolation scalar.
	 *
	 * @return Interpolated value.
	 */
	double Interpolate(
		const double A,
		const double B,
		const double Alpha)
	{
		return A + ((B - A) * Alpha);
	}

	/**
	 * @brief Floors a coordinate to a signed lattice index.
	 *
	 * Terrain frequency values used by this generator remain intentionally small,
	 * so this conversion stays well within int32 range.
	 *
	 * @param Value Coordinate to floor.
	 *
	 * @return Integer lattice coordinate.
	 */
	int32 FloorToLatticeCoordinate(const double Value)
	{
		return static_cast<int32>(std::floor(Value));
	}

	/**
	 * @brief Produces a stable unsigned hash for one three-dimensional lattice point.
	 *
	 * @param X Integer X lattice coordinate.
	 * @param Y Integer Y lattice coordinate.
	 * @param Z Integer Z lattice coordinate.
	 * @param Seed Unsigned deterministic planet seed.
	 *
	 * @return Stable pseudo-random hash value.
	 */
	uint32 HashLatticePoint(
		const int32 X,
		const int32 Y,
		const int32 Z,
		const uint32 Seed)
	{
		uint32 Hash = Seed;

		Hash ^= static_cast<uint32>(X) * 0x9E3779B9u;
		Hash ^= static_cast<uint32>(Y) * 0x85EBCA6Bu;
		Hash ^= static_cast<uint32>(Z) * 0xC2B2AE35u;

		Hash ^= Hash >> 16;
		Hash *= 0x7FEB352Du;
		Hash ^= Hash >> 15;
		Hash *= 0x846CA68Bu;
		Hash ^= Hash >> 16;

		return Hash;
	}

	/**
	 * @brief Converts a hash into a signed scalar in the range approximately -1 to +1.
	 *
	 * @param Hash Deterministic unsigned hash.
	 *
	 * @return Signed pseudo-random scalar.
	 */
	double HashToSignedUnitValue(const uint32 Hash)
	{
		constexpr double MaximumUint32 =
			static_cast<double>(MAX_uint32);

		return ((static_cast<double>(Hash) / MaximumUint32) * 2.0)
			- 1.0;
	}

	/**
	 * @brief Samples coherent three-dimensional value noise.
	 *
	 * Using world-independent 3D direction-space coordinates is deliberate:
	 * it keeps the terrain field continuous across all cube-face boundaries.
	 *
	 * @param Position Noise-space sample position.
	 * @param Seed Deterministic noise seed.
	 *
	 * @return Coherent signed value approximately in the range -1 to +1.
	 */
	double SampleValueNoise3D(
		const FVector3d& Position,
		const uint32 Seed)
	{
		const int32 X0 = FloorToLatticeCoordinate(Position.X);
		const int32 Y0 = FloorToLatticeCoordinate(Position.Y);
		const int32 Z0 = FloorToLatticeCoordinate(Position.Z);

		const int32 X1 = X0 + 1;
		const int32 Y1 = Y0 + 1;
		const int32 Z1 = Z0 + 1;

		const double AlphaX =
			SmoothStep01(Position.X - static_cast<double>(X0));

		const double AlphaY =
			SmoothStep01(Position.Y - static_cast<double>(Y0));

		const double AlphaZ =
			SmoothStep01(Position.Z - static_cast<double>(Z0));

		const double Corner000 =
			HashToSignedUnitValue(HashLatticePoint(X0, Y0, Z0, Seed));

		const double Corner100 =
			HashToSignedUnitValue(HashLatticePoint(X1, Y0, Z0, Seed));

		const double Corner010 =
			HashToSignedUnitValue(HashLatticePoint(X0, Y1, Z0, Seed));

		const double Corner110 =
			HashToSignedUnitValue(HashLatticePoint(X1, Y1, Z0, Seed));

		const double Corner001 =
			HashToSignedUnitValue(HashLatticePoint(X0, Y0, Z1, Seed));

		const double Corner101 =
			HashToSignedUnitValue(HashLatticePoint(X1, Y0, Z1, Seed));

		const double Corner011 =
			HashToSignedUnitValue(HashLatticePoint(X0, Y1, Z1, Seed));

		const double Corner111 =
			HashToSignedUnitValue(HashLatticePoint(X1, Y1, Z1, Seed));

		const double InterpolatedX00 =
			Interpolate(Corner000, Corner100, AlphaX);

		const double InterpolatedX10 =
			Interpolate(Corner010, Corner110, AlphaX);

		const double InterpolatedX01 =
			Interpolate(Corner001, Corner101, AlphaX);

		const double InterpolatedX11 =
			Interpolate(Corner011, Corner111, AlphaX);

		const double InterpolatedY0 =
			Interpolate(InterpolatedX00, InterpolatedX10, AlphaY);

		const double InterpolatedY1 =
			Interpolate(InterpolatedX01, InterpolatedX11, AlphaY);

		return Interpolate(InterpolatedY0, InterpolatedY1, AlphaZ);
	}

	/**
	 * @brief Samples layered coherent value noise.
	 *
	 * @param UnitDirection Normalised planet direction.
	 * @param Seed Deterministic base seed.
	 * @param BaseFrequency Initial direction-space frequency.
	 * @param Octaves Number of layered samples.
	 * @param Lacunarity Frequency multiplier per octave.
	 * @param Gain Amplitude multiplier per octave.
	 *
	 * @return Normalised signed fractal noise approximately in the range -1 to +1.
	 */
	double SampleFractalValueNoise3D(
		const FVector3d& UnitDirection,
		const uint32 Seed,
		const double BaseFrequency,
		const int32 Octaves,
		const double Lacunarity,
		const double Gain)
	{
		double AccumulatedValue = 0.0;
		double AccumulatedAmplitude = 0.0;
		double Frequency = BaseFrequency;
		double Amplitude = 1.0;

		for (int32 OctaveIndex = 0;
			OctaveIndex < Octaves;
			++OctaveIndex)
		{
			const uint32 OctaveSeed =
				Seed + (static_cast<uint32>(OctaveIndex) * 0x9E3779B9u);

			AccumulatedValue +=
				SampleValueNoise3D(
					UnitDirection * Frequency,
					OctaveSeed)
				* Amplitude;

			AccumulatedAmplitude += Amplitude;
			Frequency *= Lacunarity;
			Amplitude *= Gain;
		}

		return AccumulatedAmplitude > 0.0
			? AccumulatedValue / AccumulatedAmplitude
			: 0.0;
	}

	/**
	 * @brief Samples the immutable macro-terrain algorithm for generator version 1.
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

		/**
		 * Broad continental field. This determines whether a point tends towards
		 * a basin, lowland, plateau, or high continental region.
		 */
		const double Continentalness =
			SampleFractalValueNoise3D(
				UnitDirection,
				Seed ^ 0xA511E9B3u,
				1.20,
				4,
				2.05,
				0.50);

		/**
		 * Ridged noise creates mountain chains rather than uniformly lumpy ground.
		 * Squaring the ridge later gives pronounced crests while retaining smooth
		 * transitions into surrounding terrain.
		 */
		const double RidgeSource =
			SampleFractalValueNoise3D(
				UnitDirection,
				Seed ^ 0x63D83595u,
				4.75,
				4,
				2.10,
				0.52);

		const double Ridge =
			1.0 - FMath::Abs(RidgeSource);

		/**
		 * Separate lowland, mountain, and basin masks provide a readable first
		 * planet shape without inventing climate, sea level, or biome systems yet.
		 */
		const double LandMask =
			SmoothRange(-0.18, 0.36, Continentalness);

		const double MountainMask =
			SmoothRange(0.06, 0.78, Continentalness);

		const double BasinMask =
			1.0 - SmoothRange(-0.88, -0.10, Continentalness);

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