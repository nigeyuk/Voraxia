// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetImpactCratering.cpp
 * @brief Implementation of deterministic impact-crater terrain contribution sampling.
 */

#include "Terrain/Features/VoraxiaPlanetImpactCratering.h"

#include <cmath>

namespace
{
	/**
	 * @brief Floors a scalar to a signed crater-lattice coordinate.
	 *
	 * @param Value Source scalar.
	 *
	 * @return Signed lattice coordinate.
	 */
	int32 FloorToCraterLatticeCoordinate(const double Value)
	{
		return static_cast<int32>(std::floor(Value));
	}

	/**
	 * @brief Produces a stable hash for one candidate crater cell.
	 *
	 * @param X Candidate-cell X coordinate.
	 * @param Y Candidate-cell Y coordinate.
	 * @param Z Candidate-cell Z coordinate.
	 * @param Seed Deterministic crater-field seed.
	 *
	 * @return Stable unsigned pseudo-random value.
	 */
	uint32 HashCraterCell(
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
	 * @brief Converts a hash into a stable 0 to 1 scalar.
	 *
	 * @param Hash Deterministic hash value.
	 *
	 * @return Scalar in the inclusive 0 to 1 range.
	 */
	double HashToUnitValue(const uint32 Hash)
	{
		return static_cast<double>(Hash)
			/ static_cast<double>(MAX_uint32);
	}

	/**
	 * @brief Returns a decorrelated 0 to 1 value for one candidate-cell channel.
	 *
	 * @param X Candidate-cell X coordinate.
	 * @param Y Candidate-cell Y coordinate.
	 * @param Z Candidate-cell Z coordinate.
	 * @param Seed Deterministic crater-field seed.
	 * @param Channel Stable decorrelation channel.
	 *
	 * @return Stable pseudo-random scalar in the inclusive 0 to 1 range.
	 */
	double GetCandidateRandom(
		const int32 X,
		const int32 Y,
		const int32 Z,
		const uint32 Seed,
		const uint32 Channel)
	{
		return HashToUnitValue(
			HashCraterCell(
				X,
				Y,
				Z,
				Seed ^ Channel));
	}

	/**
	 * @brief Clamps a feature-profile control into the supported terrain range.
	 *
	 * @param Value Authored feature control.
	 *
	 * @return Safe non-negative contribution value.
	 */
	double ClampFeatureControl(const double Value)
	{
		return FMath::Clamp(Value, 0.0, 3.0);
	}

	/**
	 * @brief Smoothly fades a crater basin from centre to rim.
	 *
	 * @param NormalisedDistance Distance from crater centre divided by crater radius.
	 *
	 * @return Basin profile in the 0 to 1 range.
	 */
	double GetBasinProfile(const double NormalisedDistance)
	{
		if (NormalisedDistance >= 1.0)
		{
			return 0.0;
		}

		const double ClampedDistance =
			FMath::Clamp(
				NormalisedDistance,
				0.0,
				1.0);

		const double OneMinusDistanceSquared =
			1.0 - (ClampedDistance * ClampedDistance);

		return OneMinusDistanceSquared
			* OneMinusDistanceSquared;
	}

	/**
	 * @brief Produces a narrow positive profile centred on a crater rim.
	 *
	 * @param NormalisedDistance Distance from crater centre divided by crater radius.
	 *
	 * @return Rim profile in the 0 to 1 range.
	 */
	double GetRimProfile(const double NormalisedDistance)
	{
		const double RimOffset =
			(NormalisedDistance - 1.0) / 0.18;

		return std::exp(
			-(RimOffset * RimOffset));
	}

	/**
	 * @brief Produces a broad low-amplitude ejecta profile beyond a crater rim.
	 *
	 * @param NormalisedDistance Distance from crater centre divided by crater radius.
	 *
	 * @return Ejecta profile in the 0 to 1 range.
	 */
	double GetEjectaProfile(const double NormalisedDistance)
	{
		if (NormalisedDistance <= 1.0
			|| NormalisedDistance >= 2.80)
		{
			return 0.0;
		}

		const double EjectaAlpha =
			(NormalisedDistance - 1.0) / 1.80;

		const double Fade =
			1.0 - FMath::Clamp(
				EjectaAlpha,
				0.0,
				1.0);

		return Fade * Fade;
	}
}

bool VoraxiaPlanetImpactCratering::SampleImpactCratering(
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const FVector3d& UnitDirection,
	FVoraxiaPlanetImpactCrateringSample& OutSample)
{
	OutSample = FVoraxiaPlanetImpactCrateringSample();

	if (!RuntimeState.IsValid()
		|| UnitDirection.SizeSquared() <= 0.000000000001)
	{
		return false;
	}

	/**
	 * Disabled impact cratering is a valid deterministic zero contribution.
	 */
	if (!RuntimeState.ImpactCratering.IsActive())
	{
		return true;
	}

	const FVector3d SafeUnitDirection =
		UnitDirection.GetSafeNormal();

	const double Intensity =
		ClampFeatureControl(
			RuntimeState.ImpactCratering.Intensity);

	const double Frequency =
		ClampFeatureControl(
			RuntimeState.ImpactCratering.Frequency);

	if (Intensity <= 0.0 || Frequency <= 0.0)
	{
		return true;
	}

	const double FrequencyNormalised =
		FMath::Clamp(
			Frequency / 1.50,
			0.0,
			1.0);

	/**
	 * Candidate density is evaluated in global direction space. This means the
	 * crater field remains continuous across cube-face boundaries and has no
	 * dependence on cube face, streamed patch, world origin, or preview mesh.
	 */
	const double CandidateGridFrequency =
		FMath::Lerp(
			2.40,
			13.50,
			FrequencyNormalised);

	const double CandidateActivationChance =
		FMath::Lerp(
			0.20,
			0.72,
			FrequencyNormalised);

	const uint32 Seed =
		static_cast<uint32>(RuntimeState.Seed)
		^ 0x8F3C16D5u;

	const FVector3d ScaledDirection =
		SafeUnitDirection * CandidateGridFrequency;

	const int32 BaseX =
		FloorToCraterLatticeCoordinate(
			ScaledDirection.X);

	const int32 BaseY =
		FloorToCraterLatticeCoordinate(
			ScaledDirection.Y);

	const int32 BaseZ =
		FloorToCraterLatticeCoordinate(
			ScaledDirection.Z);

	bool bFoundCandidate = false;
	double BestNormalisedDistance = TNumericLimits<double>::Max();
	double BestRadius = 0.0;
	double BestSizeFactor = 0.0;

	/**
	 * Sampling the surrounding 3x3x3 cells is sufficient because the candidate
	 * centres are jittered only within their own cells. It keeps this query
	 * bounded and suitable for repeated terrain-preview evaluations.
	 */
	for (int32 OffsetZ = -1; OffsetZ <= 1; ++OffsetZ)
	{
		for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
		{
			for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
			{
				const int32 CellX = BaseX + OffsetX;
				const int32 CellY = BaseY + OffsetY;
				const int32 CellZ = BaseZ + OffsetZ;

				const double Activation =
					GetCandidateRandom(
						CellX,
						CellY,
						CellZ,
						Seed,
						0x1C7A5E2Bu);

				if (Activation > CandidateActivationChance)
				{
					continue;
				}

				const FVector3d CandidatePoint(
					static_cast<double>(CellX)
						+ GetCandidateRandom(
							CellX,
							CellY,
							CellZ,
							Seed,
							0xF21B6D43u),
					static_cast<double>(CellY)
						+ GetCandidateRandom(
							CellX,
							CellY,
							CellZ,
							Seed,
							0x6A09E667u),
					static_cast<double>(CellZ)
						+ GetCandidateRandom(
							CellX,
							CellY,
							CellZ,
							Seed,
							0xBB67AE85u));

				if (CandidatePoint.SizeSquared()
					<= 0.000000000001)
				{
					continue;
				}

				const FVector3d CandidateDirection =
					CandidatePoint.GetSafeNormal();

				const double ChordDistance =
					(CandidateDirection - SafeUnitDirection).Length();

				/**
				 * Small frequent impactors create dense fields of smaller craters.
				 * Low-frequency profiles retain fewer, larger ancient basins.
				 */
				const double RandomSize =
					GetCandidateRandom(
						CellX,
						CellY,
						CellZ,
						Seed,
						0x3C6EF372u);

				const double RadiusScale =
					FMath::Lerp(
						0.42,
						1.00,
						RandomSize * RandomSize);

				const double BaseRadius =
					FMath::Lerp(
						0.095,
						0.016,
						FrequencyNormalised);

				const double CraterRadius =
					BaseRadius * RadiusScale;

				const double NormalisedDistance =
					ChordDistance / CraterRadius;

				if (NormalisedDistance < BestNormalisedDistance)
				{
					bFoundCandidate = true;
					BestNormalisedDistance = NormalisedDistance;
					BestRadius = CraterRadius;
					BestSizeFactor = RadiusScale;
				}
			}
		}
	}

	if (!bFoundCandidate
		|| BestRadius <= 0.0
		|| BestNormalisedDistance >= 2.80)
	{
		return true;
	}

	const double SafeMaximumTerrainHeightMetres =
		FMath::Max(
			0.0,
			RuntimeState.MaximumTerrainHeightMetres);

	const double SafeMaximumTerrainDepthMetres =
		FMath::Max(
			0.0,
			RuntimeState.MaximumTerrainDepthMetres);

	const double BasinProfile =
		GetBasinProfile(
			BestNormalisedDistance);

	const double RimProfile =
		GetRimProfile(
			BestNormalisedDistance);

	const double EjectaProfile =
		GetEjectaProfile(
			BestNormalisedDistance);

	OutSample.BasinDepthMetres =
		SafeMaximumTerrainDepthMetres
		* 0.78
		* Intensity
		* BestSizeFactor
		* BasinProfile;

	OutSample.RimHeightMetres =
		SafeMaximumTerrainHeightMetres
		* 0.17
		* Intensity
		* BestSizeFactor
		* RimProfile;

	OutSample.EjectaHeightMetres =
		SafeMaximumTerrainHeightMetres
		* 0.025
		* Intensity
		* BestSizeFactor
		* EjectaProfile;

	OutSample.Craterness =
		FMath::Clamp(
			FMath::Max(
				BasinProfile,
				FMath::Max(
					RimProfile,
					EjectaProfile * 0.35)),
			0.0,
			1.0);

	return FMath::IsFinite(OutSample.BasinDepthMetres)
		&& FMath::IsFinite(OutSample.RimHeightMetres)
		&& FMath::IsFinite(OutSample.EjectaHeightMetres)
		&& FMath::IsFinite(OutSample.Craterness);
}
