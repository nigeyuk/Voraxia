// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Mining/VoraxiaMiningTypes.h"
#include "VoraxiaMiningInventoryReceiver.generated.h"

/*
 * Implemented by an actor that can receive extracted mining yields.
 * The Raptor owns delivery; voxel asteroids remain responsible only for
 * discovering and reporting what their carve removed.
 */
UINTERFACE(BlueprintType)
class VORAXIAVOXEL_API UVoraxiaMiningInventoryReceiver : public UInterface
{
	GENERATED_BODY()
};

class VORAXIAVOXEL_API IVoraxiaMiningInventoryReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voraxia|Mining")
	void ReceiveMiningYield(const FVoraxiaMiningYield& MiningYield);
};
