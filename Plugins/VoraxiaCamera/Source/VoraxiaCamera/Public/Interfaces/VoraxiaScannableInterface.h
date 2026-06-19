// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VoraxiaScannableInterface.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct VORAXIACAMERA_API FVoraxiaScanCompositionEntry
{
	GENERATED_BODY()

	/** Stable ID for the material/resource, for example Iron, Nickel, Silicate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia|Scan")
	FName MaterialId = NAME_None;

	/** Friendly name shown to the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia|Scan")
	FText DisplayName;

	/** Estimated percentage of this material in the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia|Scan", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Percentage = 0.0f;
};

UINTERFACE(BlueprintType)
class VORAXIACAMERA_API UVoraxiaScannableInterface : public UInterface
{
	GENERATED_BODY()
};

class VORAXIACAMERA_API IVoraxiaScannableInterface
{
	GENERATED_BODY()

public:
	/** Returns whether this object can currently be scanned by the requester. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Voraxia|Scan")
	bool CanBeScanned(AActor* ScanRequester) const;

	/** Display name shown in scanner UI. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Voraxia|Scan")
	FText GetScanDisplayName() const;

	/** Short scanner summary shown before detailed composition. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Voraxia|Scan")
	FText GetScanSummary() const;

	/** Composition entries for resource/material readouts. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Voraxia|Scan")
	TArray<FVoraxiaScanCompositionEntry> GetScanComposition() const;

	/** Time required to fully scan this target. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Voraxia|Scan")
	float GetScanTimeSeconds() const;
};
