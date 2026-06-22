// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VoraxiaVoxelMineable.generated.h"

UINTERFACE(BlueprintType)
class VORAXIAVOXEL_API UVoraxiaVoxelMineable : public UInterface
{
	GENERATED_BODY()
};

/*
 * Implemented by any Voraxia voxel object that can receive a real density
 * carve from a mining tool.
 *
 * The tool supplies a world-space hit result and brush radius. The target
 * owns the density-field modification and mesh rebuild.
 */
class VORAXIAVOXEL_API IVoraxiaVoxelMineable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voraxia|Voxel Mining")
	bool ApplyVoxelCarve(
		const FHitResult& Hit,
		float WorldBrushRadius
	);
};