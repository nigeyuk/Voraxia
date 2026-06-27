// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetSurfacePatchComponent.cpp
 * @brief Implementation of the local cube-sphere terrain patch-grid preview component.
 */

#include "Terrain/VoraxiaPlanetSurfacePatchComponent.h"

#include "Planet/VoraxiaPlanetActor.h"
#include "Planet/VoraxiaPlanetDefinition.h"
#include "Planet/VoraxiaPlanetMath.h"

#include "DynamicMesh/DynamicMesh3.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

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
RebuildPreviewMesh()
{
	SanitizePreviewSettings();

	if (!bGeneratePreviewMesh)
	{
		ClearPreviewMesh();
		return;
	}

	AVoraxiaPlanetActor* PlanetActor =
		Cast<AVoraxiaPlanetActor>(GetOwner());

	if (!IsValid(PlanetActor))
	{
		ClearPreviewMesh();
		return;
	}

	const FVoraxiaPlanetRuntimeState RuntimeState =
		PlanetActor->GetPlanetRuntimeState();

	if (RuntimeState.IsValid())
	{
		RefreshFromPlanetRuntimeState(RuntimeState);
		return;
	}

	UVoraxiaPlanetDefinition* PlanetDefinition =
		PlanetActor->GetPlanetDefinition();

	if (!IsValid(PlanetDefinition))
	{
		ClearPreviewMesh();
		return;
	}

	FString ValidationFailureReason;

	if (!PlanetDefinition->IsDefinitionValid(ValidationFailureReason))
	{
		ClearPreviewMesh();
		return;
	}

	/**
	 * Editor preview generation is intentionally derived from the design-time
	 * definition rather than mutating the actor's replicated runtime state.
	 *
	 * At BeginPlay, the authoritative server still validates the definition and
	 * creates the real replicated state through its normal lifecycle.
	 */
	RefreshFromPlanetRuntimeState(
		PlanetDefinition->CreateRuntimeState());
}

void UVoraxiaPlanetSurfacePatchComponent::
RefreshFromPlanetRuntimeState(
	const FVoraxiaPlanetRuntimeState& RuntimeState)
{
	SanitizePreviewSettings();

	if (!bGeneratePreviewMesh || !RuntimeState.IsValid())
	{
		ClearPreviewMesh();
		return;
	}

	const FVoraxiaPlanetChunkId BaseChunkId =
		BuildPreviewChunkId(RuntimeState);

	FDynamicMesh3 GeneratedMesh;
	bool bGeneratedAnyPatch = false;

	/**
	 * The preview deliberately merges adjacent chunks into one Dynamic Mesh.
	 *
	 * This is a visual validation tool, not the final streaming representation.
	 * Production terrain will retain independently streamed, culled, collision-
	 * bearing chunk components. Building the region in one mesh here keeps the
	 * Details-panel preview lightweight while exposing seams between addresses.
	 */
	for (int32 OffsetY = 0; OffsetY < PreviewChunkSpanY; ++OffsetY)
	{
		for (int32 OffsetX = 0; OffsetX < PreviewChunkSpanX; ++OffsetX)
		{
			FVoraxiaPlanetChunkId ChunkId = BaseChunkId;
			ChunkId.X += OffsetX;
			ChunkId.Y += OffsetY;

			if (!BuildReferenceSpherePatchMesh(
				ChunkId,
				RuntimeState,
				PreviewPatchResolution,
				PreviewPlanetRadiusCentimetres,
				GeneratedMesh))
			{
				ClearPreviewMesh();
				return;
			}

			bGeneratedAnyPatch = true;
		}
	}

	if (!bGeneratedAnyPatch)
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

#if WITH_EDITOR
void UVoraxiaPlanetSurfacePatchComponent::
PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName =
		PropertyChangedEvent.GetPropertyName();

	const bool bPreviewSettingChanged =
		PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			bGeneratePreviewMesh)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			PreviewChunkFace)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			PreviewChunkLevel)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			PreviewChunkX)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			PreviewChunkY)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			PreviewChunkSpanX)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			PreviewChunkSpanY)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			PreviewPatchResolution)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			PreviewPlanetRadiusCentimetres);

	if (bPreviewSettingChanged)
	{
		RebuildPreviewMesh();
	}
}
#endif

void UVoraxiaPlanetSurfacePatchComponent::
SanitizePreviewSettings()
{
	/**
	 * Persistent chunk addresses are currently defined through Level 24.
	 * Keep this aligned with FVoraxiaPlanetChunkId validation.
	 */
	constexpr int32 MaximumPreviewChunkLevel = 24;

	PreviewChunkLevel = FMath::Clamp(
		PreviewChunkLevel,
		0,
		MaximumPreviewChunkLevel);

	const int32 PatchesPerAxis =
		FVoraxiaPlanetChunkId::GetPatchesPerAxis(
			PreviewChunkLevel);

	PreviewChunkX = FMath::Clamp(
		PreviewChunkX,
		0,
		FMath::Max(0, PatchesPerAxis - 1));

	PreviewChunkY = FMath::Clamp(
		PreviewChunkY,
		0,
		FMath::Max(0, PatchesPerAxis - 1));

	/**
	 * Limit the visual preview region to eight chunks along either axis and
	 * keep it wholly inside the selected cube face. This is a tooling guard,
	 * not a runtime streaming limit.
	 */
	PreviewChunkSpanX = FMath::Clamp(
		PreviewChunkSpanX,
		1,
		FMath::Min(8, PatchesPerAxis - PreviewChunkX));

	PreviewChunkSpanY = FMath::Clamp(
		PreviewChunkSpanY,
		1,
		FMath::Min(8, PatchesPerAxis - PreviewChunkY));

	PreviewPatchResolution = FMath::Clamp(
		PreviewPatchResolution,
		2,
		128);

	PreviewPlanetRadiusCentimetres = FMath::Max(
		100.0,
		PreviewPlanetRadiusCentimetres);
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