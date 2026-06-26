// Copyright 2026 Coding Custard Studios.

#include "Planet/VoraxiaPlanetMath.h"

namespace
{
	constexpr double MinimumValidVectorLengthSquared = 1.0e-24;

	bool IsFiniteVector(const FVector3d& Vector)
	{
		return FMath::IsFinite(Vector.X)
			&& FMath::IsFinite(Vector.Y)
			&& FMath::IsFinite(Vector.Z);
	}

	bool IsValidReferenceRadius(const double ReferenceRadiusMetres)
	{
		return FMath::IsFinite(ReferenceRadiusMetres)
			&& ReferenceRadiusMetres > 0.0;
	}

	EVoraxiaCubeFace SelectCanonicalFaceFromUnitDirection(const FVector3d& UnitDirection)
	{
		const double AbsoluteX = FMath::Abs(UnitDirection.X);
		const double AbsoluteY = FMath::Abs(UnitDirection.Y);
		const double AbsoluteZ = FMath::Abs(UnitDirection.Z);

		/**
		 * The >= comparisons deliberately establish a deterministic tie-break:
		 *
		 * X takes precedence over Y and Z.
		 * Y takes precedence over Z.
		 *
		 * That means a direction exactly on a cube edge/corner always gets the
		 * same persistent face identity on every machine.
		 */
		if (AbsoluteX >= AbsoluteY && AbsoluteX >= AbsoluteZ)
		{
			return UnitDirection.X >= 0.0
				? EVoraxiaCubeFace::PositiveX
				: EVoraxiaCubeFace::NegativeX;
		}

		if (AbsoluteY >= AbsoluteZ)
		{
			return UnitDirection.Y >= 0.0
				? EVoraxiaCubeFace::PositiveY
				: EVoraxiaCubeFace::NegativeY;
		}

		return UnitDirection.Z >= 0.0
			? EVoraxiaCubeFace::PositiveZ
			: EVoraxiaCubeFace::NegativeZ;
	}
}

bool VoraxiaPlanetMath::GetCubeFaceBasis(
	const EVoraxiaCubeFace Face,
	FVector3d& OutFaceNormal,
	FVector3d& OutFaceUAxis,
	FVector3d& OutFaceVAxis)
{
	OutFaceNormal = FVector3d::ZeroVector;
	OutFaceUAxis = FVector3d::ZeroVector;
	OutFaceVAxis = FVector3d::ZeroVector;

	switch (Face)
	{
	case EVoraxiaCubeFace::PositiveX:
		OutFaceNormal = FVector3d(1.0, 0.0, 0.0);
		OutFaceUAxis = FVector3d(0.0, 1.0, 0.0);
		OutFaceVAxis = FVector3d(0.0, 0.0, 1.0);
		return true;

	case EVoraxiaCubeFace::NegativeX:
		OutFaceNormal = FVector3d(-1.0, 0.0, 0.0);
		OutFaceUAxis = FVector3d(0.0, -1.0, 0.0);
		OutFaceVAxis = FVector3d(0.0, 0.0, 1.0);
		return true;

	case EVoraxiaCubeFace::PositiveY:
		OutFaceNormal = FVector3d(0.0, 1.0, 0.0);
		OutFaceUAxis = FVector3d(-1.0, 0.0, 0.0);
		OutFaceVAxis = FVector3d(0.0, 0.0, 1.0);
		return true;

	case EVoraxiaCubeFace::NegativeY:
		OutFaceNormal = FVector3d(0.0, -1.0, 0.0);
		OutFaceUAxis = FVector3d(1.0, 0.0, 0.0);
		OutFaceVAxis = FVector3d(0.0, 0.0, 1.0);
		return true;

	case EVoraxiaCubeFace::PositiveZ:
		OutFaceNormal = FVector3d(0.0, 0.0, 1.0);
		OutFaceUAxis = FVector3d(1.0, 0.0, 0.0);
		OutFaceVAxis = FVector3d(0.0, 1.0, 0.0);
		return true;

	case EVoraxiaCubeFace::NegativeZ:
		OutFaceNormal = FVector3d(0.0, 0.0, -1.0);
		OutFaceUAxis = FVector3d(-1.0, 0.0, 0.0);
		OutFaceVAxis = FVector3d(0.0, 1.0, 0.0);
		return true;

	default:
		return false;
	}
}

bool VoraxiaPlanetMath::IsFaceCoordinateWithinBounds(
	const double FaceUMetres,
	const double FaceVMetres,
	const double ReferenceRadiusMetres,
	const double ToleranceMetres)
{
	if (!IsValidReferenceRadius(ReferenceRadiusMetres)
		|| !FMath::IsFinite(FaceUMetres)
		|| !FMath::IsFinite(FaceVMetres))
	{
		return false;
	}

	const double SafeToleranceMetres = FMath::Max(0.0, ToleranceMetres);
	const double MaximumCoordinateMetres = ReferenceRadiusMetres + SafeToleranceMetres;

	return FMath::Abs(FaceUMetres) <= MaximumCoordinateMetres
		&& FMath::Abs(FaceVMetres) <= MaximumCoordinateMetres;
}

bool VoraxiaPlanetMath::FaceCoordinatesToCubePoint(
	const EVoraxiaCubeFace Face,
	const double FaceUMetres,
	const double FaceVMetres,
	const double ReferenceRadiusMetres,
	FVector3d& OutCubePointMetres)
{
	OutCubePointMetres = FVector3d::ZeroVector;

	if (!IsFaceCoordinateWithinBounds(
		FaceUMetres,
		FaceVMetres,
		ReferenceRadiusMetres))
	{
		return false;
	}

	FVector3d FaceNormal;
	FVector3d FaceUAxis;
	FVector3d FaceVAxis;

	if (!GetCubeFaceBasis(Face, FaceNormal, FaceUAxis, FaceVAxis))
	{
		return false;
	}

	OutCubePointMetres =
		(FaceNormal * ReferenceRadiusMetres)
		+ (FaceUAxis * FaceUMetres)
		+ (FaceVAxis * FaceVMetres);

	return true;
}

bool VoraxiaPlanetMath::FaceCoordinatesToUnitDirection(
	const EVoraxiaCubeFace Face,
	const double FaceUMetres,
	const double FaceVMetres,
	const double ReferenceRadiusMetres,
	FVector3d& OutUnitDirection)
{
	OutUnitDirection = FVector3d::ZeroVector;

	FVector3d CubePointMetres;

	if (!FaceCoordinatesToCubePoint(
		Face,
		FaceUMetres,
		FaceVMetres,
		ReferenceRadiusMetres,
		CubePointMetres))
	{
		return false;
	}

	const double CubePointLengthSquared = CubePointMetres.SizeSquared();

	if (CubePointLengthSquared <= MinimumValidVectorLengthSquared)
	{
		return false;
	}

	OutUnitDirection = CubePointMetres / FMath::Sqrt(CubePointLengthSquared);

	return true;
}

bool VoraxiaPlanetMath::FaceCoordinatesToReferenceSurfacePositionMetres(
	const EVoraxiaCubeFace Face,
	const double FaceUMetres,
	const double FaceVMetres,
	const double ReferenceRadiusMetres,
	FVector3d& OutSurfacePositionMetres)
{
	OutSurfacePositionMetres = FVector3d::ZeroVector;

	FVector3d UnitDirection;

	if (!FaceCoordinatesToUnitDirection(
		Face,
		FaceUMetres,
		FaceVMetres,
		ReferenceRadiusMetres,
		UnitDirection))
	{
		return false;
	}

	OutSurfacePositionMetres = UnitDirection * ReferenceRadiusMetres;

	return true;
}

bool VoraxiaPlanetMath::DirectionToFaceCoordinates(
	const FVector3d& Direction,
	const double ReferenceRadiusMetres,
	EVoraxiaCubeFace& OutFace,
	double& OutFaceUMetres,
	double& OutFaceVMetres)
{
	OutFace = EVoraxiaCubeFace::Invalid;
	OutFaceUMetres = 0.0;
	OutFaceVMetres = 0.0;

	if (!IsFiniteVector(Direction)
		|| !IsValidReferenceRadius(ReferenceRadiusMetres))
	{
		return false;
	}

	const double DirectionLengthSquared = Direction.SizeSquared();

	if (DirectionLengthSquared <= MinimumValidVectorLengthSquared)
	{
		return false;
	}

	const FVector3d UnitDirection = Direction / FMath::Sqrt(DirectionLengthSquared);

	const EVoraxiaCubeFace SelectedFace = SelectCanonicalFaceFromUnitDirection(UnitDirection);

	FVector3d FaceNormal;
	FVector3d FaceUAxis;
	FVector3d FaceVAxis;

	if (!GetCubeFaceBasis(SelectedFace, FaceNormal, FaceUAxis, FaceVAxis))
	{
		return false;
	}

	const double DirectionFacingAmount = FVector3d::DotProduct(UnitDirection, FaceNormal);

	if (DirectionFacingAmount <= 0.0)
	{
		return false;
	}

	/**
	 * Scale the direction until it intersects the chosen cube-face plane.
	 *
	 * The face plane sits ReferenceRadiusMetres from the centre.
	 */
	const double RayDistanceToFacePlane = ReferenceRadiusMetres / DirectionFacingAmount;
	const FVector3d CubePointMetres = UnitDirection * RayDistanceToFacePlane;

	const double RawFaceUMetres = FVector3d::DotProduct(CubePointMetres, FaceUAxis);
	const double RawFaceVMetres = FVector3d::DotProduct(CubePointMetres, FaceVAxis);

	/**
	 * Conversion through a normalised direction can introduce microscopic edge
	 * error, so permit a very small radius-relative tolerance, then clamp.
	 */
	const double EdgeToleranceMetres = FMath::Max(
		1.0e-6,
		ReferenceRadiusMetres * 1.0e-12);

	if (!IsFaceCoordinateWithinBounds(
		RawFaceUMetres,
		RawFaceVMetres,
		ReferenceRadiusMetres,
		EdgeToleranceMetres))
	{
		return false;
	}

	OutFace = SelectedFace;
	OutFaceUMetres = FMath::Clamp(
		RawFaceUMetres,
		-ReferenceRadiusMetres,
		ReferenceRadiusMetres);

	OutFaceVMetres = FMath::Clamp(
		RawFaceVMetres,
		-ReferenceRadiusMetres,
		ReferenceRadiusMetres);

	return true;
}

bool VoraxiaPlanetMath::PlanetPositionToPlanetLocalPositionMetres(
	const FVoraxiaPlanetPosition& PlanetPosition,
	const double ReferenceRadiusMetres,
	FVector3d& OutPlanetLocalPositionMetres)
{
	OutPlanetLocalPositionMetres = FVector3d::ZeroVector;

	if (!PlanetPosition.IsValid()
		|| !IsValidReferenceRadius(ReferenceRadiusMetres))
	{
		return false;
	}

	const double RadialDistanceMetres =
		ReferenceRadiusMetres + PlanetPosition.AltitudeMetres;

	/**
	 * A position cannot pass through the centre of the planet.
	 *
	 * Underground locations are perfectly valid, provided their radius remains
	 * greater than zero.
	 */
	if (!FMath::IsFinite(RadialDistanceMetres)
		|| RadialDistanceMetres <= 0.0)
	{
		return false;
	}

	FVector3d UnitDirection;

	if (!FaceCoordinatesToUnitDirection(
		PlanetPosition.Face,
		PlanetPosition.FaceUMetres,
		PlanetPosition.FaceVMetres,
		ReferenceRadiusMetres,
		UnitDirection))
	{
		return false;
	}

	OutPlanetLocalPositionMetres = UnitDirection * RadialDistanceMetres;

	return true;
}

bool VoraxiaPlanetMath::PlanetLocalPositionMetresToPlanetPosition(
	const FVoraxiaPlanetId& PlanetId,
	const FVector3d& PlanetLocalPositionMetres,
	const double ReferenceRadiusMetres,
	FVoraxiaPlanetPosition& OutPlanetPosition)
{
	OutPlanetPosition = FVoraxiaPlanetPosition();

	if (!PlanetId.IsValid()
		|| !IsFiniteVector(PlanetLocalPositionMetres)
		|| !IsValidReferenceRadius(ReferenceRadiusMetres))
	{
		return false;
	}

	const double RadialDistanceMetres = PlanetLocalPositionMetres.Length();

	if (!FMath::IsFinite(RadialDistanceMetres)
		|| RadialDistanceMetres <= FMath::Sqrt(MinimumValidVectorLengthSquared))
	{
		return false;
	}

	EVoraxiaCubeFace Face;
	double FaceUMetres = 0.0;
	double FaceVMetres = 0.0;

	if (!DirectionToFaceCoordinates(
		PlanetLocalPositionMetres,
		ReferenceRadiusMetres,
		Face,
		FaceUMetres,
		FaceVMetres))
	{
		return false;
	}

	OutPlanetPosition.PlanetId = PlanetId;
	OutPlanetPosition.Face = Face;
	OutPlanetPosition.FaceUMetres = FaceUMetres;
	OutPlanetPosition.FaceVMetres = FaceVMetres;
	OutPlanetPosition.AltitudeMetres =
		RadialDistanceMetres - ReferenceRadiusMetres;

	return true;
}

bool VoraxiaPlanetMath::TryGetRadialUpDirection(
	const FVector3d& PlanetLocalPositionMetres,
	FVector3d& OutUpDirection)
{
	OutUpDirection = FVector3d::ZeroVector;

	if (!IsFiniteVector(PlanetLocalPositionMetres))
	{
		return false;
	}

	const double PositionLengthSquared = PlanetLocalPositionMetres.SizeSquared();

	if (PositionLengthSquared <= MinimumValidVectorLengthSquared)
	{
		return false;
	}

	OutUpDirection =
		PlanetLocalPositionMetres / FMath::Sqrt(PositionLengthSquared);

	return true;
}

bool VoraxiaPlanetMath::TryGetGravityDirection(
	const FVector3d& PlanetLocalPositionMetres,
	FVector3d& OutGravityDirection)
{
	OutGravityDirection = FVector3d::ZeroVector;

	FVector3d UpDirection;

	if (!TryGetRadialUpDirection(PlanetLocalPositionMetres, UpDirection))
	{
		return false;
	}

	OutGravityDirection = -UpDirection;

	return true;
}
