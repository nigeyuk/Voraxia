// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetSurfacePatchComponent.cpp
 * @brief Implementation of the local cube-sphere terrain region and whole-planet preview component.
 */

#include "Terrain/VoraxiaPlanetSurfacePatchComponent.h"

#include "Components/BaseDynamicMeshSceneProxy.h"

#include "Planet/VoraxiaPlanetActor.h"
#include "Planet/VoraxiaPlanetDefinition.h"
#include "Planet/VoraxiaPlanetMath.h"
#include "Terrain/VoraxiaPlanetTerrainGenerator.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMeshOverlay.h"
#include "Materials/MaterialInterface.h"
#include "UObject/SoftObjectPtr.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

namespace
{
	/**
	 * @brief Returns a stable saturated diagnostic colour from a persistent chunk address.
	 *
	 * @param ChunkId Chunk whose address should determine the preview colour.
	 * @param Intensity Brightness multiplier applied to the selected debug colour.
	 *
	 * @return Linear colour used for every vertex of that preview patch.
	 */
	FLinearColor GetChunkDebugColour(
		const FVoraxiaPlanetChunkId& ChunkId,
		const float Intensity)
	{
		/**
		 * This is deliberately a hand-picked, high-saturation engineering palette.
		 *
		 * The preview exists to make chunk boundaries and address changes obvious,
		 * not to resemble final terrain or a softened colour gradient. The fixed
		 * palette also avoids nearby chunks converging on almost-identical pastels.
		 */
		static const FLinearColor DiagnosticPalette[] =
		{
			FLinearColor(1.00f, 0.05f, 0.05f), // Signal red
			FLinearColor(1.00f, 0.28f, 0.00f), // Ember orange
			FLinearColor(1.00f, 0.85f, 0.00f), // Signal yellow
			FLinearColor(0.38f, 1.00f, 0.00f), // Acid green
			FLinearColor(0.00f, 0.85f, 0.22f), // Emerald
			FLinearColor(0.00f, 0.82f, 0.90f), // Electric cyan
			FLinearColor(0.00f, 0.30f, 1.00f), // Cobalt blue
			FLinearColor(0.34f, 0.06f, 1.00f), // Violet
			FLinearColor(0.82f, 0.00f, 1.00f), // Magenta
			FLinearColor(1.00f, 0.00f, 0.52f), // Hot pink
			FLinearColor(0.78f, 0.18f, 0.02f), // Rust
			FLinearColor(0.00f, 0.58f, 0.50f)  // Deep teal
		};

		/**
		 * These prime multipliers create a stable distinction between nearby
		 * coordinates without introducing random state. This is debug presentation
		 * only, not part of the planet generator or persistence contract.
		 */
		const uint32 AddressSeed =
			(static_cast<uint32>(ChunkId.Face) * 29u)
			+ (static_cast<uint32>(ChunkId.Level) * 47u)
			+ (static_cast<uint32>(ChunkId.X) * 71u)
			+ (static_cast<uint32>(ChunkId.Y) * 113u);

		const FLinearColor BaseColour =
			DiagnosticPalette[
				AddressSeed % UE_ARRAY_COUNT(DiagnosticPalette)];

		/**
		 * Scaling brightness keeps the palette saturated at every setting. Unlike
		 * blending towards white, lowering intensity does not recreate the pastel
		 * look this diagnostic mode is intended to avoid.
		 */
		return BaseColour * FMath::Clamp(Intensity, 0.10f, 2.00f);
	}

	/**
	 * @brief Attempts to load Unreal's vertex-colour debug material.
	 *
	 * The preview remains functional if the material is unavailable. In that
	 * case the Dynamic Mesh component falls back to its normal default rendering.
	 *
	 * @return Vertex-colour material when available, otherwise null.
	 */
	UMaterialInterface* GetVertexColourDebugMaterial()
	{
		static TSoftObjectPtr<UMaterialInterface> VertexColourMaterial(
			FSoftObjectPath(
				TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial")));

		return VertexColourMaterial.LoadSynchronous();
	}

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
	 * @param bApplyMacroTerrain Whether deterministic macro terrain should offset vertices.
	 * @param VisualTerrainHeightExaggeration Preview-only multiplier applied to sampled height.
	 * @param bUseDebugVertexColours Whether vertices should receive the supplied colour.
	 * @param DebugColour Stable preview colour for this chunk.
	 * @param OutMesh Receives generated Dynamic Mesh geometry.
	 *
	 * @return True when a valid patch mesh was generated.
	 */
	bool BuildReferenceSpherePatchMesh(
		const FVoraxiaPlanetChunkId& ChunkId,
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const int32 PatchResolution,
		const double PreviewPlanetRadiusCentimetres,
		const bool bApplyMacroTerrain,
		const double VisualTerrainHeightExaggeration,
		const bool bUseDebugVertexColours,
		const FLinearColor& DebugColour,
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

		/**
		 * Terrain amplitude is intentionally exaggerated only for this compact
		 * inspection globe. It does not alter the planet definition, replicated
		 * runtime state, deterministic terrain sample, or future world-scale mesh.
		 */
		const double SafeVisualTerrainHeightExaggeration =
			FMath::Clamp(
				VisualTerrainHeightExaggeration,
				0.0,
				250.0);

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

		TArray<int32> ColourElementIds;
		FDynamicMeshColorOverlay* ColourOverlay = nullptr;

		if (bUseDebugVertexColours)
		{
			if (!OutMesh.HasAttributes())
			{
				OutMesh.EnableAttributes();
			}

			if (!OutMesh.Attributes()->HasPrimaryColors())
			{
				OutMesh.Attributes()->EnablePrimaryColors();
			}

			ColourOverlay = OutMesh.Attributes()->PrimaryColors();

			if (ColourOverlay == nullptr)
			{
				return false;
			}

			ColourElementIds.SetNumUninitialized(
				VerticesPerAxis * VerticesPerAxis);
		}

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

		const FVector4f VertexColour(
			DebugColour.R,
			DebugColour.G,
			DebugColour.B,
			1.0f);

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
				 * Macro terrain is sampled in global direction space rather than
				 * face-local coordinates. This makes the height field continuous at
				 * every cube-face seam.
				 *
				 * The optional visual exaggeration affects only the compressed editor
				 * preview. World-scale terrain and gameplay will use the unmodified
				 * deterministic height sample.
				 */
				double TerrainHeightMetres = 0.0;

				if (bApplyMacroTerrain)
				{
					FVoraxiaPlanetTerrainSample TerrainSample;

					if (!VoraxiaPlanetTerrain::SampleMacroTerrain(
						RuntimeState,
						UnitDirection,
						TerrainSample))
					{
						return false;
					}

					TerrainHeightMetres =
						TerrainSample.HeightMetres
						* SafeVisualTerrainHeightExaggeration;
				}

				const double SurfaceRadiusMetres =
					PlanetRadiusMetres + TerrainHeightMetres;

				if (!FMath::IsFinite(SurfaceRadiusMetres)
					|| SurfaceRadiusMetres <= 0.0)
				{
					return false;
				}

				const FVector3d PlanetLocalPositionMetres =
					UnitDirection * SurfaceRadiusMetres;

				const FVector3d PreviewPositionCentimetres =
					PlanetLocalPositionMetres
					* PreviewCentimetresPerPlanetMetre;

				const int32 VertexArrayIndex =
					(VIndex * VerticesPerAxis) + UIndex;

				VertexIds[VertexArrayIndex] =
					OutMesh.AppendVertex(
						PreviewPositionCentimetres);

				if (ColourOverlay != nullptr)
				{
					ColourElementIds[VertexArrayIndex] =
						ColourOverlay->AppendElement(VertexColour);
				}
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
				const int32 FirstTriangleId =
					OutMesh.AppendTriangle(
						VertexIds[Vertex11],
						VertexIds[Vertex10],
						VertexIds[Vertex00]);

				const int32 SecondTriangleId =
					OutMesh.AppendTriangle(
						VertexIds[Vertex01],
						VertexIds[Vertex11],
						VertexIds[Vertex00]);

				if (ColourOverlay != nullptr)
				{
					if (FirstTriangleId >= 0)
					{
						ColourOverlay->SetTriangle(
							FirstTriangleId,
							UE::Geometry::FIndex3i(
								ColourElementIds[Vertex11],
								ColourElementIds[Vertex10],
								ColourElementIds[Vertex00]));
					}

					if (SecondTriangleId >= 0)
					{
						ColourOverlay->SetTriangle(
							SecondTriangleId,
							UE::Geometry::FIndex3i(
								ColourElementIds[Vertex01],
								ColourElementIds[Vertex11],
								ColourElementIds[Vertex00]));
					}
				}
			}
		}

		return OutMesh.TriangleCount() > 0;
	}

	/**
	 * @brief Builds a merged diagnostic mesh containing every patch on all six cube faces.
	 *
	 * Each patch remains independently generated from its persistent cube-face
	 * address. The resulting geometry is merged only for convenient editor
	 * inspection, not as a model for final streamed terrain ownership.
	 *
	 * @param RuntimeState Valid replicated or editor-derived planet state.
	 * @param WholePlanetPreviewLevel Quadtree level to generate across every face.
	 * @param PatchResolution Number of generated quads along every patch edge.
	 * @param PreviewPlanetRadiusCentimetres Local compressed preview radius.
	 * @param bApplyMacroTerrain Whether deterministic macro terrain should offset vertices.
	 * @param VisualTerrainHeightExaggeration Preview-only multiplier applied to sampled height.
	 * @param bUseDebugVertexColours Whether patches should receive stable debug colours.
	 * @param DebugColourIntensity Brightness multiplier applied to the diagnostic palette.
	 * @param OutMesh Receives the merged planet preview mesh.
	 *
	 * @return True when at least one valid patch was generated.
	 */
	bool BuildWholePlanetPreviewMesh(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		const int32 WholePlanetPreviewLevel,
		const int32 PatchResolution,
		const double PreviewPlanetRadiusCentimetres,
		const bool bApplyMacroTerrain,
		const double VisualTerrainHeightExaggeration,
		const bool bUseDebugVertexColours,
		const float DebugColourIntensity,
		FDynamicMesh3& OutMesh)
	{
		if (!RuntimeState.IsValid())
		{
			return false;
		}

		const int32 PatchesPerAxis =
			FVoraxiaPlanetChunkId::GetPatchesPerAxis(
				WholePlanetPreviewLevel);

		if (PatchesPerAxis <= 0)
		{
			return false;
		}

		/**
		 * Keep face iteration explicit. Persistent and network-visible cube-face
		 * identity must never depend on enum arithmetic or incidental ordering.
		 */
		static const EVoraxiaCubeFace Faces[] =
		{
			EVoraxiaCubeFace::PositiveX,
			EVoraxiaCubeFace::NegativeX,
			EVoraxiaCubeFace::PositiveY,
			EVoraxiaCubeFace::NegativeY,
			EVoraxiaCubeFace::PositiveZ,
			EVoraxiaCubeFace::NegativeZ
		};

		bool bGeneratedAnyPatch = false;

		for (const EVoraxiaCubeFace Face : Faces)
		{
			for (int32 ChunkY = 0; ChunkY < PatchesPerAxis; ++ChunkY)
			{
				for (int32 ChunkX = 0; ChunkX < PatchesPerAxis; ++ChunkX)
				{
					FVoraxiaPlanetChunkId ChunkId;

					ChunkId.PlanetId = RuntimeState.PlanetId;
					ChunkId.Face = Face;
					ChunkId.Level = WholePlanetPreviewLevel;
					ChunkId.X = ChunkX;
					ChunkId.Y = ChunkY;

					if (!BuildReferenceSpherePatchMesh(
						ChunkId,
						RuntimeState,
						PatchResolution,
						PreviewPlanetRadiusCentimetres,
						bApplyMacroTerrain,
						VisualTerrainHeightExaggeration,
						bUseDebugVertexColours,
						GetChunkDebugColour(
							ChunkId,
							DebugColourIntensity),
						OutMesh))
					{
						return false;
					}

					bGeneratedAnyPatch = true;
				}
			}
		}

		return bGeneratedAnyPatch;
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

	FDynamicMesh3 GeneratedMesh;
	bool bGeneratedAnyPatch = false;

	if (bPreviewEntirePlanet)
	{
		/**
		 * Whole-planet mode deliberately traverses every cube face and every patch
		 * at the selected diagnostic level. The merged Dynamic Mesh is for visual
		 * validation only. Production terrain will still stream, cull, collide, and
		 * persist independently managed chunks.
		 */
		bGeneratedAnyPatch = BuildWholePlanetPreviewMesh(
			RuntimeState,
			WholePlanetPreviewLevel,
			PreviewPatchResolution,
			PreviewPlanetRadiusCentimetres,
			bApplyMacroTerrain,
			VisualTerrainHeightExaggeration,
			bUseChunkDebugColours,
			DebugColourIntensity,
			GeneratedMesh);
	}
	else
	{
		const FVoraxiaPlanetChunkId BaseChunkId =
			BuildPreviewChunkId(RuntimeState);

		/**
		 * Single-face mode deliberately merges adjacent chunks into one Dynamic
		 * Mesh. This is a visual validation tool, not the final streaming
		 * representation. Building the region in one mesh keeps the Details-panel
		 * preview lightweight while exposing seams between chunk addresses.
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
					bApplyMacroTerrain,
					VisualTerrainHeightExaggeration,
					bUseChunkDebugColours,
					GetChunkDebugColour(
						ChunkId,
						DebugColourIntensity),
					GeneratedMesh))
				{
					ClearPreviewMesh();
					return;
				}

				bGeneratedAnyPatch = true;
			}
		}
	}

	if (!bGeneratedAnyPatch)
	{
		ClearPreviewMesh();
		return;
	}

	UpdatePreviewDebugMaterial();

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
			bApplyMacroTerrain)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			VisualTerrainHeightExaggeration)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			bUseChunkDebugColours)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			DebugColourIntensity)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			bPreviewEntirePlanet)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(
			UVoraxiaPlanetSurfacePatchComponent,
			WholePlanetPreviewLevel)
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
UpdatePreviewDebugMaterial()
{
	if (!bUseChunkDebugColours)
	{
		SetMaterial(0, nullptr);
		return;
	}

	/**
	 * This is an engine-supplied editor/debug material that exposes the primary
	 * vertex-colour overlay. It is not a final Voraxia surface material.
	 */
	SetMaterial(
		0,
		GetVertexColourDebugMaterial());
}

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

	DebugColourIntensity = FMath::Clamp(
		DebugColourIntensity,
		0.10f,
		2.00f);

	VisualTerrainHeightExaggeration = FMath::Clamp(
		VisualTerrainHeightExaggeration,
		0.0,
		250.0);

	/**
	 * Whole-planet mode is intentionally capped at Level 3. At the default
	 * 32-quads-per-patch preview resolution, higher levels would create enough
	 * geometry to make an editor diagnostic accidentally expensive.
	 */
	constexpr int32 MaximumWholePlanetPreviewLevel = 3;

	WholePlanetPreviewLevel = FMath::Clamp(
		WholePlanetPreviewLevel,
		0,
		MaximumWholePlanetPreviewLevel);

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
