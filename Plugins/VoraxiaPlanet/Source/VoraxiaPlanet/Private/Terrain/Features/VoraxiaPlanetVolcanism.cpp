// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetVolcanism.cpp
 * @brief Implementation of deterministic volcanic terrain contribution sampling.
 */

#include "Terrain/Features/VoraxiaPlanetVolcanism.h"

#include <cmath>

namespace
{
	/** @brief Returns a stable deterministic hash for one volcanic candidate cell. */
	uint32 HashVolcanicCell(const int32 X, const int32 Y, const int32 Z, const uint32 Seed)
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

	/** @brief Converts a deterministic hash to a scalar in the inclusive 0 to 1 range. */
	double HashToUnitValue(const uint32 Hash)
	{
		return static_cast<double>(Hash) / static_cast<double>(MAX_uint32);
	}

	/** @brief Returns one decorrelated 0 to 1 candidate-cell random channel. */
	double CandidateRandom(const int32 X, const int32 Y, const int32 Z, const uint32 Seed, const uint32 Channel)
	{
		return HashToUnitValue(HashVolcanicCell(X, Y, Z, Seed ^ Channel));
	}

	/** @brief Returns broad shield uplift emphasis. */
	double ShieldProfile(const double Distance)
	{
		if (Distance >= 2.10)
		{
			return 0.0;
		}
		const double ScaledDistance = Distance / 0.72;
		return std::exp(-(ScaledDistance * ScaledDistance));
	}

	/** @brief Returns compact central caldera emphasis. */
	double CalderaProfile(const double Distance)
	{
		if (Distance >= 0.34)
		{
			return 0.0;
		}
		const double Alpha = Distance / 0.34;
		const double Remaining = 1.0 - (Alpha * Alpha);
		return Remaining * Remaining;
	}

	/** @brief Returns a low broad lava-plain emphasis surrounding a shield. */
	double LavaPlainProfile(const double Distance)
	{
		if (Distance <= 0.50 || Distance >= 2.60)
		{
			return 0.0;
		}
		const double Alpha = (Distance - 0.50) / 2.10;
		const double Fade = 1.0 - FMath::Clamp(Alpha, 0.0, 1.0);
		return Fade * Fade;
	}
}

bool VoraxiaPlanetVolcanism::SampleVolcanism(
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const FVector3d& UnitDirection,
	FVoraxiaPlanetVolcanismSample& OutSample)
{
	OutSample = FVoraxiaPlanetVolcanismSample();

	if (!RuntimeState.IsValid() || UnitDirection.SizeSquared() <= 0.000000000001)
	{
		return false;
	}

	if (!RuntimeState.Volcanism.IsActive())
	{
		return true;
	}

	const double Intensity = FMath::Clamp(RuntimeState.Volcanism.Intensity, 0.0, 3.0);
	const double Frequency = FMath::Clamp(RuntimeState.Volcanism.Frequency, 0.0, 3.0);

	if (Intensity <= 0.0 || Frequency <= 0.0)
	{
		return true;
	}

	const FVector3d SafeDirection = UnitDirection.GetSafeNormal();
	const double FrequencyNormalised = FMath::Clamp(Frequency / 1.50, 0.0, 1.0);
	const double GridFrequency = FMath::Lerp(2.10, 12.00, FrequencyNormalised);
	const double ActivationChance = FMath::Lerp(0.18, 0.68, FrequencyNormalised);
	const uint32 Seed = static_cast<uint32>(RuntimeState.Seed) ^ 0x54D7B8A1u;
	const FVector3d ScaledDirection = SafeDirection * GridFrequency;
	const int32 BaseX = static_cast<int32>(std::floor(ScaledDirection.X));
	const int32 BaseY = static_cast<int32>(std::floor(ScaledDirection.Y));
	const int32 BaseZ = static_cast<int32>(std::floor(ScaledDirection.Z));

	bool bFoundSystem = false;
	double BestDistance = TNumericLimits<double>::Max();
	double BestScale = 0.0;
	double BestMaturity = 0.0;

	/**
	 * Candidate centres live in global direction space. This makes volcanic
	 * systems continuous across cube faces and independent of chunk identity,
	 * world origin, preview mesh, collision, networking, and persistence.
	 */
	for (int32 OffsetZ = -1; OffsetZ <= 1; ++OffsetZ)
	{
		for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
		{
			for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
			{
				const int32 X = BaseX + OffsetX;
				const int32 Y = BaseY + OffsetY;
				const int32 Z = BaseZ + OffsetZ;

				if (CandidateRandom(X, Y, Z, Seed, 0x19A4D0C7u) > ActivationChance)
				{
					continue;
				}

				const FVector3d CandidatePoint(
					static_cast<double>(X) + CandidateRandom(X, Y, Z, Seed, 0xC3EF3720u),
					static_cast<double>(Y) + CandidateRandom(X, Y, Z, Seed, 0xA54FF53Au),
					static_cast<double>(Z) + CandidateRandom(X, Y, Z, Seed, 0x510E527Fu));

				if (CandidatePoint.SizeSquared() <= 0.000000000001)
				{
					continue;
				}

				const double RadiusScale = FMath::Lerp(
					0.46,
					1.00,
					FMath::Square(CandidateRandom(X, Y, Z, Seed, 0x9B05688Cu)));
				const double BaseRadius = FMath::Lerp(0.145, 0.023, FrequencyNormalised);
				const double Distance = (CandidatePoint.GetSafeNormal() - SafeDirection).Length()
					/ (BaseRadius * RadiusScale);

				if (Distance < BestDistance)
				{
					bFoundSystem = true;
					BestDistance = Distance;
					BestScale = RadiusScale;
					BestMaturity = CandidateRandom(X, Y, Z, Seed, 0x1F83D9ABu);
				}
			}
		}
	}

	if (!bFoundSystem || BestScale <= 0.0 || BestDistance >= 2.60)
	{
		return true;
	}

	const double Shield = ShieldProfile(BestDistance);
	const double Caldera = CalderaProfile(BestDistance);
	const double LavaPlain = LavaPlainProfile(BestDistance);
	const double MaxHeight = FMath::Max(0.0, RuntimeState.MaximumTerrainHeightMetres);
	const double MaxDepth = FMath::Max(0.0, RuntimeState.MaximumTerrainDepthMetres);
	const double CalderaMaturity = FMath::Lerp(0.35, 1.00, BestMaturity);

	OutSample.ShieldUpliftMetres = MaxHeight * 0.42 * Intensity * BestScale * Shield;
	OutSample.CalderaDepthMetres = MaxDepth * 0.58 * Intensity * BestScale * CalderaMaturity * Caldera;
	OutSample.LavaPlainHeightMetres = MaxHeight * 0.055 * Intensity * BestScale * LavaPlain;
	OutSample.Volcanism = FMath::Clamp(FMath::Max(Shield, FMath::Max(Caldera, LavaPlain * 0.40)), 0.0, 1.0);

	return FMath::IsFinite(OutSample.ShieldUpliftMetres)
		&& FMath::IsFinite(OutSample.CalderaDepthMetres)
		&& FMath::IsFinite(OutSample.LavaPlainHeightMetres)
		&& FMath::IsFinite(OutSample.Volcanism);
}
