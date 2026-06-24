// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VoraxiaBlueprintDataAsset.generated.h"

UENUM(BlueprintType)
enum class EVoraxiaBlueprintCategory : uint8
{
	Ship,
	Station,
	Component,
	Thruster,
	Tool,
	Unknown
};

UCLASS(BlueprintType)
class VORAXIA_API UVoraxiaBlueprintDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blueprint")
	FName BlueprintId = "BP_UNKNOWN";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blueprint")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blueprint")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blueprint")
	EVoraxiaBlueprintCategory Category = EVoraxiaBlueprintCategory::Unknown;
};
