// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/VoraxiaFocusableInterface.h"
#include "Interfaces/VoraxiaScannableInterface.h"
#include "VoraxiaTestScannableAsteroid.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class VORAXIACAMERA_API AVoraxiaTestScannableAsteroid
	: public AActor
	, public IVoraxiaFocusableInterface
	, public IVoraxiaScannableInterface
{
	GENERATED_BODY()

public:
	AVoraxiaTestScannableAsteroid();

	virtual bool CanBeFocused_Implementation(AActor* FocusRequester) const override;
	virtual FText GetFocusDisplayName_Implementation() const override;
	virtual FVector GetFocusLocation_Implementation(AActor* FocusRequester) const override;

	virtual bool CanBeScanned_Implementation(AActor* ScanRequester) const override;
	virtual FText GetScanDisplayName_Implementation() const override;
	virtual FText GetScanSummary_Implementation() const override;
	virtual TArray<FVoraxiaScanCompositionEntry> GetScanComposition_Implementation() const override;
	virtual float GetScanTimeSeconds_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia|Test Asteroid")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Focus")
	bool bCanBeFocused = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Focus")
	FText FocusDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Focus")
	FVector FocusWorldOffset = FVector(0.0f, 0.0f, 150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Scan")
	bool bCanBeScanned = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Scan")
	FText ScanDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Scan")
	FText ScanSummary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Scan", meta=(ClampMin="0.0"))
	float ScanTimeSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Scan")
	TArray<FVoraxiaScanCompositionEntry> ScanComposition;
};
