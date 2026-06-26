// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetSurfacePatchComponent.h
 * @brief Local visual preview component for one deterministic planet terrain patch.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"

#include "Core/VoraxiaPlanetTypes.h"

#include "VoraxiaPlanetSurfacePatchComponent.generated.h"

struct FVoraxiaPlanetRuntimeState;

/**
 * @brief Builds a single cube-sphere terrain patch as a local Dynamic Mesh preview.
 *
 * This component is intentionally a visual prototype:
 *
 * - It receives replicated planet runtime state from its owning planet actor.
 * - It independently reconstructs the same patch geometry on server and clients.
 * - It does not replicate generated vertices, triangles, collision, or mesh buffers.
 * - It currently generates only a smooth reference-sphere shell with no terrain noise.
 * - It deliberately scales the real planet down into a local preview radius.
 *
 * @warning This is not the final streaming terrain component.
 *
 * The purpose of this first mesh is to validate that a real
 * FVoraxiaPlanetChunkId produces the expected cube-sphere patch geometry before
 * we add LOD, deterministic height generation, collision, edits, or replication
 * of terrain operations.
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
	 * @brief Constructs the deterministic patch address used for this preview.
	 *
	 * @param RuntimeState Valid replicated planet runtime state.
	 *
	 * @return Stable chunk identity containing the runtime planet ID.
	 */
	FVoraxiaPlanetChunkId BuildPreviewChunkId(
		const FVoraxiaPlanetRuntimeState& RuntimeState) const;

	/**
	 * @brief Tracks whether the latest refresh generated usable mesh geometry.
	 */
	bool bHasGeneratedPreviewMesh = false;
};