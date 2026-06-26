// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetDebugComponent.h
 * @brief Runtime debug visualisation component for Voraxia planets.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Core/VoraxiaPlanetTypes.h"

#include "VoraxiaPlanetDebugComponent.generated.h"

struct FVoraxiaPlanetRuntimeState;

/**
 * @brief Draws a compact visual reference of Voraxia's cube-sphere maths.
 *
 * A real planet may be thousands of kilometres wide, so this component does
 * not draw the planet at its physical scale. Instead, it draws a local,
 * deliberately small representation around the planet actor:
 *
 * - The reference sphere.
 * - The source cube used for cube-sphere projection.
 * - The six cube-face centres.
 * - Face normals and tangent axes.
 * - Inward radial gravity directions.
 * - One manually selectable surface sample marker.
 *
 * The component is intentionally local-only. It reads the replicated planet
 * runtime state but does not replicate debug geometry or add network traffic.
 */
UCLASS(ClassGroup = (Voraxia), BlueprintType, meta = (BlueprintSpawnableComponent))
class VORAXIAPLANET_API UVoraxiaPlanetDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief Creates the debug component with ticking enabled for frame-based drawing.
	 */
	UVoraxiaPlanetDebugComponent();

	/**
	 * @brief Draws the compact cube-sphere reference frame while debugging is enabled.
	 *
	 * @param DeltaTime Elapsed time since the previous component tick.
	 * @param TickType Type of engine tick currently being performed.
	 * @param ThisTickFunction Tick function instance owned by this component.
	 */
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * @brief Returns whether this component should draw its runtime visualisation.
	 *
	 * This requires both the local component option and Project Settings:
	 * Voraxia > Planet > Enable Planet Debugging.
	 *
	 * @return True when debug drawing should be active.
	 */
	UFUNCTION(BlueprintPure, Category = "Voraxia Planet|Debug")
	bool ShouldDrawDebugVisualisation() const;

	/**
	 * @brief Draws one frame of the cube-sphere reference visualisation.
	 *
	 * This function can also be called manually from C++ or Blueprint if a
	 * one-off diagnostic draw is useful.
	 */
	UFUNCTION(BlueprintCallable, Category = "Voraxia Planet|Debug")
	void DrawPlanetReferenceFrame();

protected:
	/**
	 * @brief Enables or disables this component's visual debug representation.
	 *
	 * The Project Settings master toggle must also be enabled.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug")
	bool bDrawPlanetReferenceFrame = true;

	/**
	 * @brief Draws the small reference sphere around the planet actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug")
	bool bDrawReferenceSphere = true;

	/**
	 * @brief Draws the unprojected source cube surrounding the reference sphere.
	 *
	 * This helps show the relationship between cube-face coordinates and the
	 * final spherical direction used by the planet system.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug")
	bool bDrawSourceCube = true;

	/**
	 * @brief Draws outward normal arrows from all six cube-face centres.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug")
	bool bDrawFaceNormals = true;

	/**
	 * @brief Draws the local U and V tangent axes at each cube-face centre.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug")
	bool bDrawFaceTangentAxes = true;

	/**
	 * @brief Draws inward radial gravity arrows at each cube-face centre.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug")
	bool bDrawGravityDirections = true;

	/**
	 * @brief Draws labels such as +X, -Y, and +Z at the six face centres.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug")
	bool bDrawFaceLabels = true;
	
	/**
 * @brief Draws cube-sphere terrain patch boundaries for one quadtree level.
 *
 * These curves represent the persistent face-local patch layout used by
 * FVoraxiaPlanetChunkId. The grid itself is visual-only and does not create
 * terrain, collision, actors, or network traffic.
 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug|Chunk Grid")
	bool bDrawChunkGrid = true;

	/**
	 * @brief Quadtree level displayed by the chunk-boundary grid.
	 *
	 * Level 0 displays one patch per cube face.
	 * Level 1 displays two patches per axis, or four patches per face.
	 * Level 2 displays four patches per axis, or sixteen patches per face.
	 *
	 * The visualiser is capped at Level 4 to prevent accidental excessive debug
	 * drawing. The actual runtime chunk system may support deeper levels later.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug|Chunk Grid",
		meta = (ClampMin = "0", ClampMax = "4"))
	int32 DebugChunkGridLevel = 2;

	/**
	 * @brief Number of straight debug segments used to approximate each curved
	 * cube-sphere patch boundary.
	 *
	 * Higher values make curved boundaries smoother but increase debug draw cost.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug|Chunk Grid",
		meta = (ClampMin = "2", ClampMax = "32"))
	int32 DebugChunkGridSamplesPerEdge = 12;

	/**
	 * @brief Draws the currently configured individual surface sample marker.
	 *
	 * The marker is bright orange so it remains distinct from the normal
	 * cube-face diagnostic arrows.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug|Surface Sample")
	bool bDrawSelectedSurfaceSample = true;

	/**
	 * @brief Cube face containing the selected surface sample.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug|Surface Sample")
	EVoraxiaCubeFace DebugSurfaceSampleFace = EVoraxiaCubeFace::PositiveX;

	/**
	 * @brief Horizontal U coordinate on the selected cube face, in metres.
	 *
	 * Valid values lie between -PlanetRadiusMetres and +PlanetRadiusMetres.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug|Surface Sample",
		meta = (Units = "m"))
	double DebugSurfaceSampleUMetres = 0.0;

	/**
	 * @brief Vertical V coordinate on the selected cube face, in metres.
	 *
	 * Valid values lie between -PlanetRadiusMetres and +PlanetRadiusMetres.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug|Surface Sample",
		meta = (Units = "m"))
	double DebugSurfaceSampleVMetres = 0.0;

	/**
	 * @brief Height above or below the planet reference sphere, in metres.
	 *
	 * Zero means the sample lies exactly on the reference sphere.
	 * Negative values represent underground positions.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug|Surface Sample",
		meta = (Units = "m"))
	double DebugSurfaceSampleAltitudeMetres = 0.0;

	/**
	 * @brief Draws a radial line from the preview planet centre to the sample.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug|Surface Sample")
	bool bDrawSurfaceSampleRadialLine = true;

	/**
	 * @brief Draws the selected sample's outward radial up direction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug|Surface Sample")
	bool bDrawSurfaceSampleUpDirection = true;

	/**
	 * @brief Draws the selected sample's inward radial gravity direction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug|Surface Sample")
	bool bDrawSurfaceSampleGravityDirection = true;

	/**
	 * @brief Draws a label showing the exact selected face and metre coordinates.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug|Surface Sample")
	bool bDrawSurfaceSampleLabel = true;

	/**
	 * @brief Radius of the local debug preview, expressed in Unreal centimetres.
	 *
	 * This is only a visual scale. It does not alter the planet's real radius,
	 * generation data, collision, gravity, or replicated state.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug",
		meta = (ClampMin = "100.0", Units = "cm"))
	double DebugPreviewRadiusCentimetres = 3000.0;

	/**
	 * @brief Radius of the selected orange sample marker, in Unreal centimetres.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug|Surface Sample",
		meta = (ClampMin = "5.0", Units = "cm"))
	double DebugSurfaceSampleMarkerRadiusCentimetres = 35.0;

	/**
	 * @brief Length of normal and gravity arrows relative to preview radius.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug",
		meta = (ClampMin = "0.05", ClampMax = "2.0"))
	double DebugArrowLengthRatio = 0.35;

	/**
	 * @brief Number of segments used for the reference sphere.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug",
		meta = (ClampMin = "8", ClampMax = "64"))
	int32 DebugSphereSegments = 24;

	/**
	 * @brief Thickness used for diagnostic lines and arrows.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Debug",
		meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float DebugLineThickness = 2.0f;

private:
	
	/**
 * @brief Draws the selected quadtree-level grid across all six cube faces.
 *
 * @param World World receiving frame-based debug line draws.
 * @param Origin Local preview origin at the planet actor.
 * @param RuntimeState Valid replicated planet state.
 * @param PreviewRadiusCentimetres Radius of the local visual planet preview.
 */
void DrawChunkGrid(
	UWorld* World,
	const FVector& Origin,
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	double PreviewRadiusCentimetres) const;

/**
 * @brief Draws all U and V patch-boundary curves for one cube face.
 *
 * @param World World receiving frame-based debug line draws.
 * @param Origin Local preview origin at the planet actor.
 * @param RuntimeState Valid replicated planet state.
 * @param Face Cube face being drawn.
 * @param GridLevel Quadtree level represented by the visual grid.
 * @param PreviewRadiusCentimetres Radius of the local visual planet preview.
 * @param SamplesPerEdge Number of line segments per curved boundary.
 */
void DrawFaceChunkGrid(
	UWorld* World,
	const FVector& Origin,
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	EVoraxiaCubeFace Face,
	int32 GridLevel,
	double PreviewRadiusCentimetres,
	int32 SamplesPerEdge) const;

/**
 * @brief Draws one curved line across a cube face after sphere projection.
 *
 * @param World World receiving frame-based debug line draws.
 * @param Origin Local preview origin at the planet actor.
 * @param Face Cube face being sampled.
 * @param StartUMetres Start U coordinate on the face, in metres.
 * @param StartVMetres Start V coordinate on the face, in metres.
 * @param EndUMetres End U coordinate on the face, in metres.
 * @param EndVMetres End V coordinate on the face, in metres.
 * @param ReferenceRadiusMetres Planet reference radius, in metres.
 * @param PreviewRadiusCentimetres Radius of the local visual planet preview.
 * @param SamplesPerEdge Number of line segments approximating the curve.
 * @param LineColour Debug colour used for this face's patch boundaries.
 * @param LineThickness Debug line thickness.
 */
void DrawFaceCoordinateCurve(
	UWorld* World,
	const FVector& Origin,
	EVoraxiaCubeFace Face,
	double StartUMetres,
	double StartVMetres,
	double EndUMetres,
	double EndVMetres,
	double ReferenceRadiusMetres,
	double PreviewRadiusCentimetres,
	int32 SamplesPerEdge,
	const FColor& LineColour,
	float LineThickness) const;
	/**
	 * @brief Resolves the selected debug sample into a radial planet direction.
	 *
	 * @param RuntimeState Valid replicated planet state.
	 * @param OutUnitDirection Receives the outward normalised planet direction.
	 * @param OutRadialDistanceMetres Receives the sample distance from planet centre.
	 *
	 * @return True when the selected face, coordinates, and altitude are valid.
	 */
	bool TryGetSelectedSurfaceSample(
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		FVector3d& OutUnitDirection,
		double& OutRadialDistanceMetres) const;

	/**
	 * @brief Draws the selected orange surface sample visualisation.
	 *
	 * @param World World receiving frame-based debug shapes.
	 * @param Origin Local preview origin at the planet actor.
	 * @param RuntimeState Valid replicated planet state.
	 * @param PreviewRadiusCentimetres Radius of the local visual planet preview.
	 * @param ArrowLengthCentimetres Standard debug-arrow length.
	 * @param ArrowHeadSize Arrow-head size for debug arrows.
	 */
	void DrawSelectedSurfaceSample(
		UWorld* World,
		const FVector& Origin,
		const FVoraxiaPlanetRuntimeState& RuntimeState,
		double PreviewRadiusCentimetres,
		double ArrowLengthCentimetres,
		float ArrowHeadSize) const;
};