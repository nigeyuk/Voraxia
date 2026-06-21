// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoraxiaVoxelSphereActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class VORAXIAVOXEL_API AVoraxiaVoxelSphereActor : public AActor
{
	GENERATED_BODY()

public:
	AVoraxiaVoxelSphereActor();

	virtual void BeginPlay() override;

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
};