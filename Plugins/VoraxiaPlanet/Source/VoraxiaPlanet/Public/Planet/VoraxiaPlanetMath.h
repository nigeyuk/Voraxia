// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"

#include "Core/VoraxiaPlanetTypes.h"

/**
 * Mathematical conversion helpers for Voraxia's cube-sphere planets.
 *
 * Coordinate convention:
 *
 * - Planet-space positions are expressed in metres using FVector3d.
 * - The planet centre is always FVector3d::ZeroVector in planet-local space.
 * - A cube face is a square plane exactly ReferenceRadiusMetres from the centre.
 * - Face U and V coordinates are measured in metres along that face's tangent axes.
 * - Valid U/V coordinates run from -ReferenceRadiusMetres to +ReferenceRadiusMetres.
 * - A cube point is projected onto the planet's reference sphere by normalising it.
 *
 * This mapping is part of the durable planet-generation contract. Do not change it
 * for an existing generator version once worlds, terrain edits, or saves exist.
 */
namespace VoraxiaPlanetMath
{
	/**
	 * Returns the outward normal and two tangent axes for a cube face.
	 *
	 * The tangent axes are chosen so:
	 *
	 * Cross(UAxis, VAxis) == FaceNormal
	 *
	 * This gives every face a stable, right-handed coordinate system.
	 */
	VORAXIAPLANET_API bool GetCubeFaceBasis(
		EVoraxiaCubeFace Face,
		FVector3d& OutFaceNormal,
		FVector3d& OutFaceUAxis,
		FVector3d& OutFaceVAxis);

	/**
	 * Checks whether U/V lie inside a cube face of the supplied reference radius.
	 *
	 * ToleranceMetres exists for harmless floating-point edge noise when converting
	 * a direction back into a face coordinate.
	 */
	VORAXIAPLANET_API bool IsFaceCoordinateWithinBounds(
		double FaceUMetres,
		double FaceVMetres,
		double ReferenceRadiusMetres,
		double ToleranceMetres = 0.0);

	/**
	 * Converts a face coordinate to an unprojected point on the cube.
	 *
	 * Example on the +X face:
	 *
	 * CubePoint = (Radius, U, V)
	 */
	VORAXIAPLANET_API bool FaceCoordinatesToCubePoint(
		EVoraxiaCubeFace Face,
		double FaceUMetres,
		double FaceVMetres,
		double ReferenceRadiusMetres,
		FVector3d& OutCubePointMetres);

	/**
	 * Converts a face coordinate into the unit direction from the planet centre.
	 *
	 * This is the cube-sphere projection step:
	 *
	 * NormalisedCubePoint = CubePoint / Length(CubePoint)
	 */
	VORAXIAPLANET_API bool FaceCoordinatesToUnitDirection(
		EVoraxiaCubeFace Face,
		double FaceUMetres,
		double FaceVMetres,
		double ReferenceRadiusMetres,
		FVector3d& OutUnitDirection);

	/**
	 * Converts a face coordinate into a point on the planet's reference sphere.
	 *
	 * The result is planet-local, in metres, relative to the planet centre.
	 */
	VORAXIAPLANET_API bool FaceCoordinatesToReferenceSurfacePositionMetres(
		EVoraxiaCubeFace Face,
		double FaceUMetres,
		double FaceVMetres,
		double ReferenceRadiusMetres,
		FVector3d& OutSurfacePositionMetres);

	/**
	 * Converts a direction into a canonical cube face and face coordinates.
	 *
	 * Edge and corner directions are mathematically shared by multiple faces.
	 * To keep saves and networking deterministic, this function chooses one:
	 *
	 * X faces take priority over Y faces.
	 * Y faces take priority over Z faces.
	 *
	 * Positive/negative is then selected from the dominant component's sign.
	 */
	VORAXIAPLANET_API bool DirectionToFaceCoordinates(
		const FVector3d& Direction,
		double ReferenceRadiusMetres,
		EVoraxiaCubeFace& OutFace,
		double& OutFaceUMetres,
		double& OutFaceVMetres);

	/**
	 * Converts a persistent planet position into a planet-local 3D position.
	 *
	 * The returned location is in metres relative to the planet centre.
	 * It is NOT an Unreal world location and is NOT expressed in centimetres.
	 */
	VORAXIAPLANET_API bool PlanetPositionToPlanetLocalPositionMetres(
		const FVoraxiaPlanetPosition& PlanetPosition,
		double ReferenceRadiusMetres,
		FVector3d& OutPlanetLocalPositionMetres);

	/**
	 * Converts a planet-local 3D position into a stable persistent planet address.
	 *
	 * PlanetId must be supplied because a raw local position cannot identify which
	 * planet it belongs to by itself.
	 */
	VORAXIAPLANET_API bool PlanetLocalPositionMetresToPlanetPosition(
		const FVoraxiaPlanetId& PlanetId,
		const FVector3d& PlanetLocalPositionMetres,
		double ReferenceRadiusMetres,
		FVoraxiaPlanetPosition& OutPlanetPosition);

	/**
	 * Returns the outward radial direction from the planet centre.
	 *
	 * This is the local "up" vector for a spherical planet.
	 */
	VORAXIAPLANET_API bool TryGetRadialUpDirection(
		const FVector3d& PlanetLocalPositionMetres,
		FVector3d& OutUpDirection);

	/**
	 * Returns the inward radial gravity direction toward the planet centre.
	 *
	 * This is direction only. The force magnitude comes later from the planet
	 * definition's SurfaceGravityMetresPerSecondSquared value.
	 */
	VORAXIAPLANET_API bool TryGetGravityDirection(
		const FVector3d& PlanetLocalPositionMetres,
		FVector3d& OutGravityDirection);
}