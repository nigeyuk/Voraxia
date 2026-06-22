// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mining/VoraxiaVoxelMineable.h"
#include "VoraxiaVoxelSphereActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class VORAXIAVOXEL_API AVoraxiaVoxelSphereActor
	: public AActor
	, public IVoraxiaVoxelMineable
{
	GENERATED_BODY()

public:
	AVoraxiaVoxelSphereActor();

	virtual void BeginPlay() override;
	
	virtual bool ApplyVoxelCarve_Implementation(
	const FHitResult& Hit,
	float WorldBrushRadius
	) override;

	/*
	 * Resets the density field to a clean sphere and rebuilds its mesh.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voraxia|Voxel Sphere")
	void BuildVoxelSphere();

	/*
	 * Removes material using a spherical subtraction brush.
	 *
	 * WorldRadius is interpreted in Unreal world units, assuming the actor
	 * remains at uniform scale. Keep this actor at scale 1,1,1 for now.
	 */
	UFUNCTION(BlueprintCallable, Category = "Voraxia|Voxel Sphere|Editing")
	bool CarveSphereAtWorldLocation(
		const FVector& WorldLocation,
		float WorldRadius
	);

	/*
	 * Removes material using asteroid-local coordinates.
	 *
	 * This is the core voxel edit function. The Raptor will eventually call
	 * the world-space version above.
	 */
	UFUNCTION(BlueprintCallable, Category = "Voraxia|Voxel Sphere|Editing")
	bool CarveSphereAtLocalPosition(
		const FVector& LocalLocation,
		float LocalRadius
	);

	/*
	 * Editor-only convenience test. Set the Debug Carve properties in Details,
	 * then click this button to make a genuine voxel crater.
	 */
	UFUNCTION(CallInEditor, Category = "Voraxia|Voxel Sphere|Debug")
	void CarveDebugSphere();
	
	/*
	* Immediately scans the density field and removes small solid islands that
	* are no longer connected to the asteroid's main body.
	*/
	UFUNCTION(
		BlueprintCallable,
		CallInEditor,
		Category = "Voraxia|Voxel Sphere|Detached Fragments"
	)
	void CleanupDetachedFragmentsNow();

	UFUNCTION(BlueprintPure, Category = "Voraxia|Voxel Sphere")
	UProceduralMeshComponent* GetVoxelMesh() const
	{
		return VoxelMesh;
	}

protected:
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UProceduralMeshComponent> VoxelMesh;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Generation",
		meta = (ClampMin = "8", ClampMax = "64", UIMin = "8", UIMax = "48")
	)
	int32 CellsPerAxis = 32;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Generation",
		meta = (ClampMin = "1.0", UIMin = "10.0", UIMax = "100.0")
	)
	float VoxelSize = 50.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Generation",
		meta = (ClampMin = "1.0")
	)
	float SphereRadius = 700.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Generation"
	)
	bool bCreateCollision = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Generation"
	)
	bool bBuildOnBeginPlay = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Appearance"
	)
	TObjectPtr<UMaterialInterface> VoxelMaterial;

	UPROPERTY(
		EditAnywhere,
		Category = "Voraxia|Voxel Sphere|Debug"
	)
	FVector DebugCarveLocalPosition = FVector(0.0f, 0.0f, 600.0f);

	UPROPERTY(
		EditAnywhere,
		Category = "Voraxia|Voxel Sphere|Debug",
		meta = (ClampMin = "1.0")
	)
	float DebugCarveRadius = 180.0f;
	
	UPROPERTY(
	EditAnywhere,
	BlueprintReadOnly,
	Category = "Voraxia|Voxel Sphere|Detached Fragments"
)
	bool bCleanupDetachedFragments = true;

	/*
	 * Each fresh mining hit restarts this delay. That lets a small fragment
	 * remain visible briefly before it is removed with its collision.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Detached Fragments",
		meta = (
			ClampMin = "0.0",
			UIMin = "0.0",
			UIMax = "10.0",
			EditCondition = "bCleanupDetachedFragments"
		)
	)
	float DetachedFragmentCleanupDelay = 3.0f;

	/*
	 * A disconnected solid region at or below this many voxel cells is
	 * considered disposable mining debris.
	 *
	 * The largest solid island is always protected.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Detached Fragments",
		meta = (
			ClampMin = "1",
			UIMin = "1",
			UIMax = "128",
			EditCondition = "bCleanupDetachedFragments"
		)
	)
	int32 MaxDetachedFragmentCellCount = 24;
	
	UPROPERTY(
	EditAnywhere,
	BlueprintReadOnly,
	Category = "Voraxia|Voxel Sphere|Detached Fragments|Debug"
)
	bool bDrawDetachedFragmentCandidates = true;

	/*
	 * When enabled, candidate fragments are boxed but not removed.
	 * Useful for checking whether the connected-island scan has found the
	 * pieces you expect before allowing cleanup to delete them.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Detached Fragments|Debug",
		meta = (EditCondition = "bDrawDetachedFragmentCandidates")
	)
	bool bDebugOnlyDetachedFragmentCandidates = false;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Detached Fragments|Debug",
		meta = (
			ClampMin = "0.1",
			UIMin = "0.1",
			UIMax = "15.0",
			EditCondition = "bDrawDetachedFragmentCandidates"
		)
	)
	float DetachedFragmentDebugLifetime = 5.0f;
	
	/*
 * Shrinks the rock used by the connectivity scan without changing the
 * rendered asteroid itself. A higher value breaks weak voxel bridges more
 * aggressively when identifying detached islands.
 *
 * This is measured in the same world-unit space as voxel density.
 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Detached Fragments|Debug",
		meta = (
			ClampMin = "0.0",
			UIMin = "0.0",
			UIMax = "50.0"
		)
	)
	float DetachedFragmentConnectivityErosion = 10.0f;

	/*
	 * Draw every disconnected island found by the scan:
	 * Cyan    = main asteroid body
	 * Orange  = detached and small enough for deletion
	 * Magenta = detached, but currently above the deletion-size limit
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia|Voxel Sphere|Detached Fragments|Debug"
	)
	bool bDrawAllDetachedFragmentIslands = true;

private:
	TArray<float> DensitySamples;

	void GenerateSphereDensity();

	bool RebuildMeshFromDensity();

	bool HasValidDensityField() const;

	FVector GetSampleLocalPosition(
		int32 X,
		int32 Y,
		int32 Z
	) const;

	int32 GetDensityIndex(
		int32 X,
		int32 Y,
		int32 Z,
		int32 SampleCount
	) const;
	
private:
	
	FTimerHandle DetachedFragmentCleanupTimerHandle;

	void ScheduleDetachedFragmentCleanup();

	void CleanupDetachedFragments();

	/*
	 * Returns the number of density cells removed from small detached islands.
	 */
	int32 RemoveSmallDetachedSolidIslands();

	bool IsCellSolid(
		int32 X,
		int32 Y,
		int32 Z
	) const;

	int32 GetCellIndex(
		int32 X,
		int32 Y,
		int32 Z
	) const;
	
	void DrawDetachedFragmentCandidateBox(
	const TArray<int32>& IslandCells,
	const FColor& Colour
	) const;
};