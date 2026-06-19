// Copyright 2026 Coding Custard Studios.

#include "VoraxiaTestScannableAsteroid.h"

#include "Components/StaticMeshComponent.h"

AVoraxiaTestScannableAsteroid::AVoraxiaTestScannableAsteroid()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	Tags.AddUnique(TEXT("CameraFocusTarget"));

	FocusDisplayName = NSLOCTEXT(
		"VoraxiaCamera",
		"TestScannableAsteroidFocusName",
		"Scannable Test Asteroid"
	);

	ScanDisplayName = NSLOCTEXT(
		"VoraxiaCamera",
		"TestScannableAsteroidScanName",
		"Carbonaceous Training Rock"
	);

	ScanSummary = NSLOCTEXT(
		"VoraxiaCamera",
		"TestScannableAsteroidScanSummary",
		"A test asteroid containing pretend mineral data for scanner validation."
	);

	FVoraxiaScanCompositionEntry Silicates;
	Silicates.MaterialId = TEXT("Silicates");
	Silicates.DisplayName = NSLOCTEXT("VoraxiaCamera", "ScanMaterialSilicates", "Silicates");
	Silicates.Percentage = 62.0f;

	FVoraxiaScanCompositionEntry Iron;
	Iron.MaterialId = TEXT("Iron");
	Iron.DisplayName = NSLOCTEXT("VoraxiaCamera", "ScanMaterialIron", "Iron");
	Iron.Percentage = 18.0f;

	FVoraxiaScanCompositionEntry Nickel;
	Nickel.MaterialId = TEXT("Nickel");
	Nickel.DisplayName = NSLOCTEXT("VoraxiaCamera", "ScanMaterialNickel", "Nickel");
	Nickel.Percentage = 4.0f;

	FVoraxiaScanCompositionEntry WaterIce;
	WaterIce.MaterialId = TEXT("WaterIce");
	WaterIce.DisplayName = NSLOCTEXT("VoraxiaCamera", "ScanMaterialWaterIce", "Water Ice");
	WaterIce.Percentage = 9.0f;

	FVoraxiaScanCompositionEntry Unknown;
	Unknown.MaterialId = TEXT("Unknown");
	Unknown.DisplayName = NSLOCTEXT("VoraxiaCamera", "ScanMaterialUnknown", "Unknown");
	Unknown.Percentage = 7.0f;

	ScanComposition = {
		Silicates,
		Iron,
		Nickel,
		WaterIce,
		Unknown
	};
}

bool AVoraxiaTestScannableAsteroid::CanBeFocused_Implementation(AActor* FocusRequester) const
{
	return bCanBeFocused;
}

FText AVoraxiaTestScannableAsteroid::GetFocusDisplayName_Implementation() const
{
	return FocusDisplayName;
}

FVector AVoraxiaTestScannableAsteroid::GetFocusLocation_Implementation(AActor* FocusRequester) const
{
	return GetActorLocation() + FocusWorldOffset;
}

bool AVoraxiaTestScannableAsteroid::CanBeScanned_Implementation(AActor* ScanRequester) const
{
	return bCanBeScanned;
}

FText AVoraxiaTestScannableAsteroid::GetScanDisplayName_Implementation() const
{
	return ScanDisplayName;
}

FText AVoraxiaTestScannableAsteroid::GetScanSummary_Implementation() const
{
	return ScanSummary;
}

TArray<FVoraxiaScanCompositionEntry> AVoraxiaTestScannableAsteroid::GetScanComposition_Implementation() const
{
	return ScanComposition;
}

float AVoraxiaTestScannableAsteroid::GetScanTimeSeconds_Implementation() const
{
	return ScanTimeSeconds;
}

