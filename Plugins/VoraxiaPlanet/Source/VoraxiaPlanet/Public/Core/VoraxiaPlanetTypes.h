// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"

#include "VoraxiaPlanetTypes.generated.h"

/**
 * The six faces of the cube used to construct a cube-sphere planet.
 *
 * The numeric values are deliberately explicit and must never be reordered.
 * They become part of our persistent chunk identity and, later, network data.
 */
UENUM(BlueprintType)
enum class EVoraxiaCubeFace : uint8
{
	PositiveX = 0 UMETA(DisplayName = "+X"),
	NegativeX = 1 UMETA(DisplayName = "-X"),
	PositiveY = 2 UMETA(DisplayName = "+Y"),
	NegativeY = 3 UMETA(DisplayName = "-Y"),
	PositiveZ = 4 UMETA(DisplayName = "+Z"),
	NegativeZ = 5 UMETA(DisplayName = "-Z"),

	Invalid = 255 UMETA(Hidden)
};

/**
 * Shared helper functions and constants for the Voraxia planet system.
 */
namespace VoraxiaPlanet
{
	/**
	 * Returns true only for one of the six real cube faces.
	 */
	FORCEINLINE bool IsValidCubeFace(const EVoraxiaCubeFace Face)
	{
		return Face >= EVoraxiaCubeFace::PositiveX
			&& Face <= EVoraxiaCubeFace::NegativeZ;
	}
}

/**
 * Explicit unit conversions used by the planet system.
 *
 * Unreal-space gameplay remains centimetre based.
 * Persistent planet-space data remains metre based.
 */
namespace VoraxiaPlanetUnits
{
	constexpr double CentimetresPerMetre = 100.0;
	constexpr double MetresPerKilometre = 1000.0;
	constexpr double CentimetresPerKilometre = CentimetresPerMetre * MetresPerKilometre;

	FORCEINLINE constexpr double MetresToCentimetres(const double Metres)
	{
		return Metres * CentimetresPerMetre;
	}

	FORCEINLINE constexpr double CentimetresToMetres(const double Centimetres)
	{
		return Centimetres / CentimetresPerMetre;
	}

	FORCEINLINE constexpr double KilometresToMetres(const double Kilometres)
	{
		return Kilometres * MetresPerKilometre;
	}

	FORCEINLINE constexpr double MetresToKilometres(const double Metres)
	{
		return Metres / MetresPerKilometre;
	}

	/**
	 * Converts a nearby planet-relative position from metres to Unreal centimetres.
	 *
	 * This does not perform origin rebasing or planet-to-local-world conversion.
	 * It only converts units.
	 */
	FORCEINLINE FVector3d MetresToUnrealCentimetres(const FVector3d& PositionMetres)
	{
		return PositionMetres * CentimetresPerMetre;
	}

	/**
	 * Converts a nearby Unreal-space position from centimetres to metres.
	 *
	 * This does not perform origin rebasing or local-world-to-planet conversion.
	 * It only converts units.
	 */
	FORCEINLINE FVector3d UnrealCentimetresToMetres(const FVector3d& PositionCentimetres)
	{
		return PositionCentimetres / CentimetresPerMetre;
	}
}

/**
 * A persistent, globally unique identity for a planet.
 *
 * It is intentionally not an actor pointer, level name, or Unreal world location.
 * A planet ID remains valid in saves, replication data, and on dedicated servers.
 */
USTRUCT(BlueprintType)
struct VORAXIAPLANET_API FVoraxiaPlanetId
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FGuid Value;

	FVoraxiaPlanetId() = default;

	explicit FVoraxiaPlanetId(const FGuid& InValue)
		: Value(InValue)
	{
	}

	static FVoraxiaPlanetId CreateNew()
	{
		return FVoraxiaPlanetId(FGuid::NewGuid());
	}

	bool IsValid() const
	{
		return Value.IsValid();
	}

	bool operator==(const FVoraxiaPlanetId& Other) const
	{
		return Value == Other.Value;
	}

	bool operator!=(const FVoraxiaPlanetId& Other) const
	{
		return !(*this == Other);
	}
};

FORCEINLINE uint32 GetTypeHash(const FVoraxiaPlanetId& PlanetId)
{
	return GetTypeHash(PlanetId.Value);
}

/**
 * Identifies one square terrain patch on one cube-sphere face.
 *
 * Level 0 means one patch covers the whole cube face.
 * Each additional level divides every patch into four children.
 *
 * Example:
 * Level 0: 1 x 1 patch per face
 * Level 1: 2 x 2 patches per face
 * Level 2: 4 x 4 patches per face
 */
USTRUCT(BlueprintType)
struct VORAXIAPLANET_API FVoraxiaPlanetChunkId
{
	GENERATED_BODY()

public:
	/**
	 * Keeps 1 << Level safely within a signed int32 and gives us extremely
	 * fine subdivision without committing the terrain renderer to a final size.
	 */
	static constexpr int32 MaxSupportedLevel = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Chunk")
	FVoraxiaPlanetId PlanetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Chunk")
	EVoraxiaCubeFace Face = EVoraxiaCubeFace::Invalid;

	/**
	 * Cube-face quadtree level.
	 * 0 is the entire face. Larger values are smaller, more detailed patches.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Chunk", meta = (ClampMin = "0", ClampMax = "24"))
	int32 Level = INDEX_NONE;

	/**
	 * Horizontal patch coordinate at this Level.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Chunk")
	int32 X = INDEX_NONE;

	/**
	 * Vertical patch coordinate at this Level.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Chunk")
	int32 Y = INDEX_NONE;

	static int32 GetPatchesPerAxis(const int32 InLevel)
	{
		if (InLevel < 0 || InLevel > MaxSupportedLevel)
		{
			return 0;
		}

		return 1 << InLevel;
	}

	bool IsValid() const
	{
		const int32 PatchesPerAxis = GetPatchesPerAxis(Level);

		return PlanetId.IsValid()
			&& VoraxiaPlanet::IsValidCubeFace(Face)
			&& PatchesPerAxis > 0
			&& X >= 0
			&& Y >= 0
			&& X < PatchesPerAxis
			&& Y < PatchesPerAxis;
	}

	bool operator==(const FVoraxiaPlanetChunkId& Other) const
	{
		return PlanetId == Other.PlanetId
			&& Face == Other.Face
			&& Level == Other.Level
			&& X == Other.X
			&& Y == Other.Y;
	}

	bool operator!=(const FVoraxiaPlanetChunkId& Other) const
	{
		return !(*this == Other);
	}
};

FORCEINLINE uint32 GetTypeHash(const FVoraxiaPlanetChunkId& ChunkId)
{
	uint32 Result = GetTypeHash(ChunkId.PlanetId);

	Result = HashCombine(Result, GetTypeHash(static_cast<uint8>(ChunkId.Face)));
	Result = HashCombine(Result, GetTypeHash(ChunkId.Level));
	Result = HashCombine(Result, GetTypeHash(ChunkId.X));
	Result = HashCombine(Result, GetTypeHash(ChunkId.Y));

	return Result;
}

/**
 * A stable address on or near a planet.
 *
 * FaceUMetres and FaceVMetres are coordinates on the unprojected cube-face plane.
 * They are NOT Unreal world coordinates.
 *
 * AltitudeMetres is measured from the planet's reference sphere, not from the
 * current generated terrain surface. It may be negative for underground spaces.
 */
USTRUCT(BlueprintType)
struct VORAXIAPLANET_API FVoraxiaPlanetPosition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Position")
	FVoraxiaPlanetId PlanetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Position")
	EVoraxiaCubeFace Face = EVoraxiaCubeFace::Invalid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Position")
	double FaceUMetres = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Position")
	double FaceVMetres = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Position")
	double AltitudeMetres = 0.0;

	FVector2d GetFaceCoordinatesMetres() const
	{
		return FVector2d(FaceUMetres, FaceVMetres);
	}

	bool IsValid() const
	{
		return PlanetId.IsValid()
			&& VoraxiaPlanet::IsValidCubeFace(Face)
			&& FMath::IsFinite(FaceUMetres)
			&& FMath::IsFinite(FaceVMetres)
			&& FMath::IsFinite(AltitudeMetres);
	}
};