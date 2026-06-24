// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProceduralMeshComponent.h"

struct FVoraxiaVoxelMeshData
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	void Reset();
};

class FVoraxiaMarchingCubes final
{
public:
	/*
	 * Builds an iso-surface at density zero.
	 *
	 * Convention:
	 *   Positive density = solid.
	 *   Negative density = empty.
	 */
	static bool BuildIsoSurface(
		const TArray<float>& DensitySamples,
		int32 CellsPerAxis,
		float VoxelSize,
		FVoraxiaVoxelMeshData& OutMesh
	);

	/*
	 * Builds one mesh-data set per material tag.
	 *
	 * Each Marching Cubes cell assigns its generated surface triangles to the
	 * material tag stored for that cell. Vertices are intentionally not shared
	 * between different material sections, because Unreal procedural mesh
	 * sections own independent vertex buffers.
	 */
	static bool BuildIsoSurfaceByMaterial(
		const TArray<float>& DensitySamples,
		int32 CellsPerAxis,
		float VoxelSize,
		const TArray<FGameplayTag>& CellMaterialTags,
		FGameplayTag FallbackMaterialTag,
		TMap<FGameplayTag, FVoraxiaVoxelMeshData>& OutMeshes
	);
};
