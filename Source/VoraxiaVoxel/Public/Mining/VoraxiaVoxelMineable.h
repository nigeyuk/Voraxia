// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Mining/VoraxiaMiningTypes.h"
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
 * The target owns density modification, remeshing, and resource discovery.
 * The tool owns delivery of yielded resources to its operator.
 */
class VORAXIAVOXEL_API IVoraxiaVoxelMineable
{
	GENERATED_BODY()

public:
	/* Legacy-compatible carve path for callers that only need success/failure. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voraxia|Voxel Mining")
	bool ApplyVoxelCarve(
		const FHitResult& Hit,
		float WorldBrushRadius
	);

	/* Preferred carve path used by mining tools that need extracted resources. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voraxia|Voxel Mining")
	bool ApplyVoxelCarveWithYields(
		const FHitResult& Hit,
		float WorldBrushRadius,
		UPARAM(ref) TArray<FVoraxiaMiningYield>& OutYields
	);
};
