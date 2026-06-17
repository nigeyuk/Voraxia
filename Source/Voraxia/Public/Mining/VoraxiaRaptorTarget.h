// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VoraxiaRaptorTarget.generated.h"

UINTERFACE(BlueprintType)
class VORAXIA_API UVoraxiaRaptorTarget : public UInterface
{
	GENERATED_BODY()
};

class VORAXIA_API IVoraxiaRaptorTarget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voraxia|Raptor")
	void ApplyRaptorMining(const FHitResult& Hit, float MiningPower, float DeltaSeconds);
};
