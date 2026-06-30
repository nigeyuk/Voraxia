// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetTerrainNoise.cpp
 * @brief Implementation of deterministic direction-space terrain noise helpers.
 */

#include "Terrain/VoraxiaPlanetTerrainNoise.h"

#include <cmath>

namespace
{
	/**
	 * @brief Floors a coordinate to a signed lattice index.
	 *
	 * @param Value Source coordinate.
	 *
	 * @return Signed lattice coordinate.
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
	 * @param Seed Unsigned deterministic field seed.
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
	 * @brief Converts a hash into a signed scalar in the approximate -1 to +1 range.
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
	 * @brief Interpolates linearly between two scalar values.
	 *
	 * @param A First scalar.
	 * @param B Second scalar.
	 * @param Alpha Interpolation scalar.
	 *
	 * @return Interpolated scalar.
	 */
	double Interpolate(
		const double A,
		const double B,
		const double Alpha)
	{
		return A + ((B - A) * Alpha);
	}
}

double VoraxiaPlanetTerrainNoise::Clamp01(const double Value)
{
	return FMath::Clamp(Value, 0.0, 1.0);
}

double VoraxiaPlanetTerrainNoise::SmoothStep01(const double Value)
{
	const double ClampedValue = Clamp01(Value);

	return ClampedValue
		* ClampedValue
		* (3.0 - (2.0 * ClampedValue));
}

double VoraxiaPlanetTerrainNoise::SmoothRange(
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

double VoraxiaPlanetTerrainNoise::SampleValueNoise3D(
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

double VoraxiaPlanetTerrainNoise::SampleFractalValueNoise3D(
	const FVector3d& UnitDirection,
	const uint32 Seed,
	const double BaseFrequency,
	const int32 Octaves,
	const double Lacunarity,
	const double Gain)
{
	if (UnitDirection.SizeSquared() <= 0.000000000001
		|| !FMath::IsFinite(BaseFrequency)
		|| !FMath::IsFinite(Lacunarity)
		|| !FMath::IsFinite(Gain)
		|| BaseFrequency <= 0.0
		|| Octaves <= 0
		|| Lacunarity <= 0.0
		|| Gain <= 0.0)
	{
		return 0.0;
	}

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

double VoraxiaPlanetTerrainNoise::SampleRidgedFractalNoise3D(
	const FVector3d& UnitDirection,
	const uint32 Seed,
	const double BaseFrequency,
	const int32 Octaves,
	const double Lacunarity,
	const double Gain)
{
	const double SignedNoise = SampleFractalValueNoise3D(
		UnitDirection,
		Seed,
		BaseFrequency,
		Octaves,
		Lacunarity,
		Gain);

	return Clamp01(1.0 - FMath::Abs(SignedNoise));
}
