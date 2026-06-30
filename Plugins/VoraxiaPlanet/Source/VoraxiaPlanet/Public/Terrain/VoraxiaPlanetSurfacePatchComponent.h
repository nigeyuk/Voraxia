// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetSurfacePatchComponent.h
 * @brief Local visual preview component for a deterministic grid of planet terrain patches.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"

#include "Core/VoraxiaPlanetTypes.h"

#include "VoraxiaPlanetSurfacePatchComponent.generated.h"

class UMaterialInterface;
struct FPropertyChangedEvent;
struct FVoraxiaPlanetRuntimeState;

/**
 * @brief Selects the local vertex-colour diagnostic used by the terrain preview.
 *
 * All modes are presentation-only. They derive their values from deterministic
 * planet data but never alter terrain generation, replicated state, persistence,
 * collision, or gameplay materials.
 */
UENUM(BlueprintType)
enum class EVoraxiaPlanetPreviewDebugColourMode : uint8
{
	/** @brief Assigns a stable saturated colour from the persistent chunk address. */
	ChunkAddress UMETA(DisplayName = "Chunk Address"),

	/** @brief Colours terrain by sampled elevation relative to the reference sphere. */
	TerrainHeight UMETA(DisplayName = "Terrain Height"),

	/** @brief Colours terrain by estimated macro-terrain steepness. */
	TerrainSlope UMETA(DisplayName = "Terrain Slope")
};

/**
 * @brief Builds a deterministic cube-sphere terrain-patch grid as one local Dynamic Mesh preview.
 *
 * This component is intentionally a visual prototype:
 *
 * - It receives replicated planet runtime state from its owning planet actor.
 * - It independently reconstructs the same patch-grid geometry on server and clients.
 * - It does not replicate generated vertices, triangles, collision, or mesh buffers.
 * - It can optionally apply deterministic macro-terrain height from the planet generator.
 * - It can colour vertices by chunk address, sampled terrain height, or macro slope.
 * - It can generate either one adjacent patch region or all six cube faces.
 * - It deliberately scales the real planet down into a local preview radius.
 *
 * @warning This is not the final streaming terrain component.
 *
 * The purpose of this first mesh is to validate that a real
 * FVoraxiaPlanetChunkId, and its selected adjacent region, produce the expected
 * cube-sphere patch geometry before we add streaming LOD, collision, edits, or
 * replication of terrain operations.
 */
UCLASS(
	ClassGroup = (Voraxia),
	BlueprintType,
	meta = (BlueprintSpawnableComponent))
class VORAXIAPLANET_API UVoraxiaPlanetSurfacePatchComponent
	: public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief Creates the component and configures it as a local visual preview.
	 */
	UVoraxiaPlanetSurfacePatchComponent();

	/**
	 * @brief Rebuilds this preview mesh from validated server-authored planet state.
	 *
	 * This function must be called by the owning planet actor after the server
	 * creates runtime state, and again on clients when that state replicates.
	 *
	 * @param RuntimeState Valid compact state replicated by AVoraxiaPlanetActor.
	 */
	void RefreshFromPlanetRuntimeState(
		const FVoraxiaPlanetRuntimeState& RuntimeState);

	/**
	 * @brief Rebuilds the preview using live runtime state or the editor definition.
	 *
	 * In play, this prefers the server-authored runtime state owned by the
	 * planet actor. In the editor, before play begins, it derives temporary
	 * preview state from the assigned planet definition asset.
	 *
	 * This function does not modify replicated planet state, persistent terrain
	 * data, or generated terrain edits.
	 */
	UFUNCTION(
		CallInEditor,
		Category = "Voraxia Planet|Terrain Preview")
	void RebuildPreviewMesh();

	/**
	 * @brief Removes all generated preview geometry from this component.
	 */
	void ClearPreviewMesh();

	/**
	 * @brief Returns whether this component currently holds generated patch geometry.
	 *
	 * @return True when the most recent build produced a valid preview mesh.
	 */
	UFUNCTION(BlueprintPure, Category = "Voraxia Planet|Terrain Preview")
	bool HasGeneratedPreviewMesh() const
	{
		return bHasGeneratedPreviewMesh;
	}

#if WITH_EDITOR
	/**
	 * @brief Rebuilds the local editor preview when preview settings change.
	 *
	 * @param PropertyChangedEvent Description of the edited Details-panel property.
	 */
	virtual void PostEditChangeProperty(
		FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/**
	 * @brief Enables or disables this local terrain preview mesh.
	 *
	 * Disabling this does not alter planet data, networking, terrain persistence,
	 * collision, or future streamed terrain chunks.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview")
	bool bGeneratePreviewMesh = true;

	/**
	 * @brief Applies deterministic macro-terrain height to preview vertices.
	 *
	 * Terrain height is sampled from the immutable runtime state and global
	 * planet direction, making it continuous across cube-face and chunk seams.
	 * This affects only locally generated preview geometry, never replicated
	 * terrain data, persistent edits, or the planet definition itself.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Macro Terrain")
	bool bApplyMacroTerrain = true;

	/**
	 * @brief Preview-only multiplier applied to macro terrain height.
	 *
	 * A true-scale planet has mountain ranges that are visually tiny when
	 * compressed into a 30-metre diagnostic globe. This multiplier exaggerates
	 * only the radial terrain displacement after deterministic sampling.
	 *
	 * @warning This is an editor and local-preview presentation control. It must
	 * never be used as part of terrain persistence, replication, collision, or
	 * gameplay coordinates.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Macro Terrain",
		meta = (
			ClampMin = "0.0",
			ClampMax = "250.0",
			UIMin = "0.0",
			UIMax = "100.0"))
	double VisualTerrainHeightExaggeration = 40.0;

	/**
	 * @brief Enables local vertex-colour diagnostic shading for the preview mesh.
	 *
	 * The selected colour mode can visualise persistent chunk addresses, sampled
	 * terrain elevation, or estimated terrain steepness. It is an editor and
	 * preview aid only, never biome data, gameplay material data, or persistent
	 * world state.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Debug",
		meta = (DisplayName = "Use Debug Colours"))
	bool bUseChunkDebugColours = true;
	
	/**
	 * @brief Selects the deterministic diagnostic used for preview vertex colours.
	 *
	 * Chunk Address keeps patch boundaries immediately visible. Terrain Height
	 * reveals basins, lowlands, highlands, and summits. Terrain Slope estimates
	 * macro steepness from the same seed-driven height field using local tangent
	 * probes, which is more expensive and is best inspected at whole-planet
	 * preview Levels 1 and 2.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Debug",
		meta = (EditCondition = "bUseChunkDebugColours"))
	EVoraxiaPlanetPreviewDebugColourMode DebugColourMode =
		EVoraxiaPlanetPreviewDebugColourMode::ChunkAddress;

	/**
	 * @brief Brightness multiplier applied to the selected debug-colour mode.
	 *
	 * This scales the chosen diagnostic colours without changing sampled terrain
	 * values. It affects only local preview rendering.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Debug",
		meta = (
			ClampMin = "0.10",
			ClampMax = "2.00",
			UIMin = "0.10",
			UIMax = "1.25"))
	float DebugColourIntensity = 1.00f;

	/**
	 * @brief Slope angle mapped to the strongest terrain-slope debug colour.
	 *
	 * This is a visual remapping control only. Lower values increase contrast
	 * for broad planetary landforms, allowing mountain belts and rift shoulders
	 * to reach yellow and red without changing sampled terrain, mesh geometry,
	 * collision, persistence, networking, or gameplay slope calculations.
	 *
	 * A value near 20 degrees is useful for the current kilometre-scale macro
	 * terrain diagnostic. Future voxel-scale previews may use a wider range.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Debug",
		meta = (
			DisplayName = "Terrain Slope Visual Range",
			ClampMin = "1.0",
			ClampMax = "89.0",
			UIMin = "5.0",
			UIMax = "45.0",
			Units = "deg",
			EditCondition = "bUseChunkDebugColours && DebugColourMode == EVoraxiaPlanetPreviewDebugColourMode::TerrainSlope",
			EditConditionHides))
	double TerrainSlopeVisualRangeDegrees = 20.0;

	/**
	 * @brief Optional authored material used when preview debug colours are disabled.
	 *
	 * Assign a simple clay, rock, or other diagnostic material here to inspect
	 * macro terrain shape under ordinary lighting. When Use Debug Colours is
	 * enabled, the component deliberately overrides this material with Unreal's
	 * vertex-colour debug material so height, slope, and chunk diagnostics remain
	 * visible.
	 *
	 * Leaving this unset restores the Dynamic Mesh component's default material.
	 * This is local preview presentation only and never affects replicated terrain,
	 * persistence, collision, or final streamed-terrain materials.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Material",
		meta = (
			DisplayName = "Preview Surface Material",
			EditCondition = "!bUseChunkDebugColours",
			EditConditionHides))
	TObjectPtr<UMaterialInterface> PreviewSurfaceMaterial = nullptr;

	/**
	 * @brief Generates a diagnostic preview containing every cube face of the planet.
	 *
	 * When enabled, the component ignores the single-face address and span
	 * settings below, then generates every patch on all six cube faces at
	 * WholePlanetPreviewLevel. This is an editor and diagnostic preview only.
	 * It does not change streaming, networking, terrain persistence, or gameplay
	 * terrain representation.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Whole Planet")
	bool bPreviewEntirePlanet = false;

	/**
	 * @brief Quadtree level used when generating the entire diagnostic planet.
	 *
	 * Level 0 produces six patches, one per cube face. Level 1 produces
	 * twenty-four patches. Level 2 produces ninety-six patches. Level 3 produces
	 * three hundred and eighty-four patches and is deliberately the maximum for
	 * this merged editor preview.
	 *
	 * @warning This guard prevents a diagnostic setting from accidentally creating
	 * millions of preview triangles. It is not a production terrain LOD limit.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Whole Planet",
		meta = (ClampMin = "0", ClampMax = "3", EditCondition = "bPreviewEntirePlanet"))
	int32 WholePlanetPreviewLevel = 1;

	/**
	 * @brief Cube face used by the patch preview.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Chunk")
	EVoraxiaCubeFace PreviewChunkFace = EVoraxiaCubeFace::PositiveX;

	/**
	 * @brief Quadtree level of the preview patch.
	 *
	 * Level 0 represents one complete cube face.
	 * Level 1 represents one quarter of a face.
	 * Level 2 represents one sixteenth of a face.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Chunk",
		meta = (ClampMin = "0", ClampMax = "24"))
	int32 PreviewChunkLevel = 0;

	/**
	 * @brief Horizontal patch coordinate at PreviewChunkLevel.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Chunk",
		meta = (ClampMin = "0"))
	int32 PreviewChunkX = 0;

	/**
	 * @brief Vertical patch coordinate at PreviewChunkLevel.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Chunk",
		meta = (ClampMin = "0"))
	int32 PreviewChunkY = 0;

	/**
	 * @brief Number of adjacent chunks generated in the positive U direction.
	 *
	 * A value of one renders only the selected chunk. Larger values render a
	 * rectangular preview region beginning at PreviewChunkX. The range is
	 * clamped so it never crosses the current cube-face boundary.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Chunk",
		meta = (ClampMin = "1", ClampMax = "8"))
	int32 PreviewChunkSpanX = 1;

	/**
	 * @brief Number of adjacent chunks generated in the positive V direction.
	 *
	 * A value of one renders only the selected chunk. Larger values render a
	 * rectangular preview region beginning at PreviewChunkY. The range is
	 * clamped so it never crosses the current cube-face boundary.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Chunk",
		meta = (ClampMin = "1", ClampMax = "8"))
	int32 PreviewChunkSpanY = 1;

	/**
	 * @brief Number of generated quads along each patch edge.
	 *
	 * A value of 32 creates 33 x 33 vertices for the outward-facing planet patch.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Mesh",
		meta = (ClampMin = "2", ClampMax = "128"))
	int32 PreviewPatchResolution = 32;

	/**
	 * @brief Radius of the compressed local planet preview, in Unreal centimetres.
	 *
	 * This is visual-only. The mesh is generated from the real planet radius in
	 * metres, then compressed so it can be inspected around the planet actor.
	 *
	 * The default matches the debug component's 30 m preview radius.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview|Mesh",
		meta = (ClampMin = "100.0", Units = "cm"))
	double PreviewPlanetRadiusCentimetres = 3000.0;

private:
	/**
	 * @brief Constructs the base deterministic patch address used for this preview.
	 *
	 * @param RuntimeState Valid replicated planet runtime state.
	 *
	 * @return Stable base chunk identity containing the runtime planet ID.
	 */
	FVoraxiaPlanetChunkId BuildPreviewChunkId(
		const FVoraxiaPlanetRuntimeState& RuntimeState) const;

	/**
	 * @brief Applies either the vertex-colour diagnostic material or the authored preview material.
	 *
	 * Debug-colour modes deliberately use Unreal's supplied vertex-colour material.
	 * When debug colours are disabled, this applies PreviewSurfaceMaterial instead.
	 * Final terrain rendering will use separate Voraxia terrain materials later.
	 */
	void UpdatePreviewDebugMaterial();

	/**
	 * @brief Clamps the editable preview address to a valid chunk coordinate range.
	 *
	 * This keeps Level, X, and Y coherent when users alter them from the
	 * Details panel or a tooling workflow.
	 */
	void SanitizePreviewSettings();

	/**
	 * @brief Tracks whether the latest refresh generated usable mesh geometry.
	 */
	bool bHasGeneratedPreviewMesh = false;
};
