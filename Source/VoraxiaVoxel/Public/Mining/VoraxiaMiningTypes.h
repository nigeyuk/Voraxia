// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "VoraxiaMiningTypes.generated.h"

/*
 * Lightweight resource amount emitted by a mineable voxel target after a
 * successful carve. Values remain float during this prototype phase so ore
 * Data Assets can use designer-friendly values such as 0.85.
 */
USTRUCT(BlueprintType)
struct VORAXIAVOXEL_API FVoraxiaMiningYield
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voraxia|Mining")
	FGameplayTag ResourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voraxia|Mining")
	float Amount = 0.0f;
};
