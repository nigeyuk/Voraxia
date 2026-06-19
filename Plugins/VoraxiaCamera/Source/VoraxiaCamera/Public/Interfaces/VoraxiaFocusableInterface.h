// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VoraxiaFocusableInterface.generated.h"

class AActor;

UINTERFACE(BlueprintType)
class VORAXIACAMERA_API UVoraxiaFocusableInterface : public UInterface
{
	GENERATED_BODY()
};

class VORAXIACAMERA_API IVoraxiaFocusableInterface
{
	GENERATED_BODY()

public:
	/** Returns whether this object can currently be focused by the requester. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Voraxia|Focus")
	bool CanBeFocused(AActor* FocusRequester) const;

	/** Display name shown in focus debug/UI. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Voraxia|Focus")
	FText GetFocusDisplayName() const;

	/** World-space point the camera should look at when focusing this object. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Voraxia|Focus")
	FVector GetFocusLocation(AActor* FocusRequester) const;
};
