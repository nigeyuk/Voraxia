// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetSurfacePatchComponent.cpp
 * @brief Implementation of the local cube-sphere terrain patch preview component.
 */

#include "Terrain/VoraxiaPlanetSurfacePatchComponent.h"

#include "Planet/VoraxiaPlanetDefinition.h"
#include "Planet/VoraxiaPlanetMath.h"

#include "DynamicMesh/DynamicMesh3.h"

namespace
{
	/**
	 * @brief Builds one smooth cube-sphere patch from a stable chunk identity.
	 *
	 * The generated mesh contains no noise, biomes, voxel density, caves, or
	 * editable terrain data. Every vertex lies directly on the planet's
	 * reference sphere.
	 *
	 * @param ChunkId Stable cube-face quadtree patch address.
	 * @param RuntimeState Valid replicated planet state.
	 * @param PatchResolution Number of generated quads along each patch edge.
	 * @param PreviewPlanetRadiusCentimetres Local compressed preview radius.
	 * @param OutMesh Receives generated Dynamic Mesh geometry.
	 *
	 * @return True when a valid patch mesh was generated.
	 */
	bool BuildReferenceSpherePatchMesh(
		const FVoraxiaPlanetChunkId& ChunkId,
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const int32 PatchResolution,
		const double PreviewPlanetRadiusCentimetres,
		FDynamicMesh3& OutMesh)
	{
		if (!ChunkId.IsValid() || !RuntimeState.IsValid())
		{
			return false;
		}

		const int32 PatchesPerAxis =
			FVoraxiaPlanetChunkId::GetPatchesPerAxis(ChunkId.Level);

		if (PatchesPerAxis <= 0)
		{
			return false;
		}

		const int32 SafePatchResolution =
			FMath::Clamp(PatchResolution, 2, 128);

		const double SafePreviewRadiusCentimetres =
			FMath::Max(100.0, PreviewPlanetRadiusCentimetres);

		const double PlanetRadiusMetres =
			RuntimeState.RadiusMetres;

		/**
		 * One Level 0 face spans from -Radius to +Radius in both U and V.
		 *
		 * Every additional quadtree level divides the face into 2^Level
		 * patches along each axis.
		 */
		const double PatchWidthMetres =
			(PlanetRadiusMetres * 2.0)
			/ static_cast<double>(PatchesPerAxis);

		const double MinimumUMetres =
			-PlanetRadiusMetres
			+ (static_cast<double>(ChunkId.X) * PatchWidthMetres);

		const double MinimumVMetres =
			-PlanetRadiusMetres
			+ (static_cast<double>(ChunkId.Y) * PatchWidthMetres);

		const double MaximumUMetres =
			MinimumUMetres + PatchWidthMetres;

		const double MaximumVMetres =
			MinimumVMetres + PatchWidthMetres;

		const int32 VerticesPerAxis =
			SafePatchResolution + 1;

		TArray<int32> VertexIds;
		VertexIds.SetNumUninitialized(
			VerticesPerAxis * VerticesPerAxis);

		/**
		 * This is deliberately a preview conversion:
		 *
		 * Real planet-space metres are compressed into nearby Unreal
		 * centimetres so we can inspect the patch around the planet actor.
		 *
		 * It does not define final world placement or origin-rebasing logic.
		 */
		const double PreviewCentimetresPerPlanetMetre =
			SafePreviewRadiusCentimetres / PlanetRadiusMetres;

		for (int32 VIndex = 0; VIndex < VerticesPerAxis; ++VIndex)
		{
			const double VAlpha =
				static_cast<double>(VIndex)
				/ static_cast<double>(SafePatchResolution);

			const double FaceVMetres =
				FMath::Lerp(
					MinimumVMetres,
					MaximumVMetres,
					VAlpha);

			for (int32 UIndex = 0; UIndex < VerticesPerAxis; ++UIndex)
			{
				const double UAlpha =
					static_cast<double>(UIndex)
					/ static_cast<double>(SafePatchResolution);

				const double FaceUMetres =
					FMath::Lerp(
						MinimumUMetres,
						MaximumUMetres,
						UAlpha);

				FVector3d UnitDirection;

				if (!VoraxiaPlanetMath::FaceCoordinatesToUnitDirection(
					ChunkId.Face,
					FaceUMetres,
					FaceVMetres,
					PlanetRadiusMetres,
					UnitDirection))
				{
					return false;
				}

				/**
				 * This is the first actual terrain-surface rule:
				 *
				 * Position = CubeSphereDirection * ReferenceRadius
				 *
				 * Later, deterministic terrain height is added radially here.
				 * For now, height is exactly zero everywhere.
				 */
				const FVector3d PlanetLocalPositionMetres =
					UnitDirection * PlanetRadiusMetres;

				const FVector3d PreviewPositionCentimetres =
					PlanetLocalPositionMetres
					* PreviewCentimetresPerPlanetMetre;

				const int32 VertexArrayIndex =
					(VIndex * VerticesPerAxis) + UIndex;

				VertexIds[VertexArrayIndex] =
					OutMesh.AppendVertex(
						PreviewPositionCentimetres);
			}
		}

		for (int32 VIndex = 0; VIndex < SafePatchResolution; ++VIndex)
		{
			for (int32 UIndex = 0;
				UIndex < SafePatchResolution;
				++UIndex)
			{
				const int32 Vertex00 =
					(VIndex * VerticesPerAxis) + UIndex;

				const int32 Vertex10 =
					(VIndex * VerticesPerAxis) + (UIndex + 1);

				const int32 Vertex11 =
					((VIndex + 1) * VerticesPerAxis) + (UIndex + 1);

				const int32 Vertex01 =
					((VIndex + 1) * VerticesPerAxis) + UIndex;

				/**
				 * The first winding order tested as inward-facing when rendered by this
				 * Dynamic Mesh preview. These triangles are intentionally reversed so the
				 * visible face points away from the planet centre.
				 *
				 * A planet surface should be visible from outside the planet and culled
				 * from inside it.
				 *
				 * @warning Do not add duplicate reversed triangles here.
				 *
				 * This preview must contain exactly two triangles per quad. Adding both
				 * winding directions creates overlapping geometry and z-fighting.
				 */
				OutMesh.AppendTriangle(
					VertexIds[Vertex11],
					VertexIds[Vertex10],
					VertexIds[Vertex00]);

				OutMesh.AppendTriangle(
					VertexIds[Vertex01],
					VertexIds[Vertex11],
					VertexIds[Vertex00]);
			}
		}

		return OutMesh.TriangleCount() > 0;
	}
}

UVoraxiaPlanetSurfacePatchComponent::
UVoraxiaPlanetSurfacePatchComponent()
{
	/**
	 * This component renders locally generated geometry only.
	 *
	 * The server replicates compact planet state, not mesh vertices or Dynamic
	 * Mesh buffers. Every machine rebuilds this diagnostic mesh from that state.
	 */
	SetIsReplicated(false);

	/**
	 * Collision is intentionally disabled for this preview-only surface.
	 *
	 * Real terrain collision will be generated per streamed gameplay chunk,
	 * governed by server authority and terrain-edit revision state.
	 */
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetCastShadow(false);
}

void UVoraxiaPlanetSurfacePatchComponent::
RefreshFromPlanetRuntimeState(
	const FVoraxiaPlanetRuntimeState& RuntimeState)
{
	if (!bGeneratePreviewMesh || !RuntimeState.IsValid())
	{
		ClearPreviewMesh();
		return;
	}

	const FVoraxiaPlanetChunkId PreviewChunkId =
		BuildPreviewChunkId(RuntimeState);

	FDynamicMesh3 GeneratedMesh;

	if (!BuildReferenceSpherePatchMesh(
		PreviewChunkId,
		RuntimeState,
		PreviewPatchResolution,
		PreviewPlanetRadiusCentimetres,
		GeneratedMesh))
	{
		ClearPreviewMesh();
		return;
	}

	/**
	 * SetMesh replaces the component's local renderable Dynamic Mesh.
	 *
	 * This runs after server initialisation and client replication, never as a
	 * replicated mesh payload.
	 */
	SetMesh(MoveTemp(GeneratedMesh));

	bHasGeneratedPreviewMesh = true;
}

void UVoraxiaPlanetSurfacePatchComponent::
ClearPreviewMesh()
{
	FDynamicMesh3 EmptyMesh;

	SetMesh(MoveTemp(EmptyMesh));

	bHasGeneratedPreviewMesh = false;
}

FVoraxiaPlanetChunkId UVoraxiaPlanetSurfacePatchComponent::
BuildPreviewChunkId(
	const FVoraxiaPlanetRuntimeState& RuntimeState) const
{
	FVoraxiaPlanetChunkId ChunkId;

	ChunkId.PlanetId = RuntimeState.PlanetId;
	ChunkId.Face = PreviewChunkFace;
	ChunkId.Level = PreviewChunkLevel;
	ChunkId.X = PreviewChunkX;
	ChunkId.Y = PreviewChunkY;

	return ChunkId;
}