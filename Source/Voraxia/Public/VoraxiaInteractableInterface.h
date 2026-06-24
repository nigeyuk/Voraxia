// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VoraxiaInteractableInterface.generated.h"

UINTERFACE(BlueprintType)
class VORAXIA_API UVoraxiaInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class VORAXIA_API IVoraxiaInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voraxia|Interaction")
	void Interact(AActor* InteractingActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voraxia|Interaction")
	FText GetInteractionText() const;
};
