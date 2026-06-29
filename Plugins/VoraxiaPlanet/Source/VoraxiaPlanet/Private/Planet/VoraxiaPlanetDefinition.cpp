// Copyright 2026 Coding Custard Studios.

#include "Planet/VoraxiaPlanetDefinition.h"

#include "Planet/VoraxiaPlanetFeatureProfile.h"

const FPrimaryAssetType UVoraxiaPlanetDefinition::PrimaryAssetType(TEXT("VoraxiaPlanetDefinition"));

FPrimaryAssetId UVoraxiaPlanetDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

void UVoraxiaPlanetDefinition::GenerateNewPlanetId()
{
	PlanetId = FVoraxiaPlanetId::CreateNew();
	MarkPackageDirty();
}

bool UVoraxiaPlanetDefinition::IsDefinitionValid(FString& OutFailureReason) const
{
	OutFailureReason.Reset();

	if (!PlanetId.IsValid())
	{
		OutFailureReason = TEXT("PlanetId is invalid. Generate a permanent Planet ID before using this definition.");
		return false;
	}

	if (GeneratorVersion <= 0)
	{
		OutFailureReason = TEXT("GeneratorVersion must be greater than zero.");
		return false;
	}

	if (!IsValid(FeatureProfile))
	{
		OutFailureReason =
			TEXT("FeatureProfile must be assigned before using this planet definition.");

		return false;
	}

	FString FeatureProfileValidationFailureReason;

	if (!FeatureProfile->IsProfileValid(FeatureProfileValidationFailureReason))
	{
		OutFailureReason = FString::Printf(
			TEXT("FeatureProfile '%s' is invalid: %s"),
			*FeatureProfile->GetName(),
			*FeatureProfileValidationFailureReason);

		return false;
	}

	if (!FeatureProfile->GetPrimaryAssetId().IsValid())
	{
		OutFailureReason = FString::Printf(
			TEXT("FeatureProfile '%s' does not have a valid primary asset ID."),
			*FeatureProfile->GetName());

		return false;
	}

	if (!FMath::IsFinite(RadiusMetres) || RadiusMetres <= 0.0)
	{
		OutFailureReason = TEXT("RadiusMetres must be a finite value greater than zero.");
		return false;
	}

	if (!FMath::IsFinite(MaximumTerrainHeightMetres) || MaximumTerrainHeightMetres < 0.0)
	{
		OutFailureReason = TEXT("MaximumTerrainHeightMetres must be finite and cannot be negative.");
		return false;
	}

	if (!FMath::IsFinite(MaximumTerrainDepthMetres) || MaximumTerrainDepthMetres < 0.0)
	{
		OutFailureReason = TEXT("MaximumTerrainDepthMetres must be finite and cannot be negative.");
		return false;
	}

	if (!FMath::IsFinite(SurfaceGravityMetresPerSecondSquared)
		|| SurfaceGravityMetresPerSecondSquared <= 0.0)
	{
		OutFailureReason = TEXT("SurfaceGravityMetresPerSecondSquared must be a finite value greater than zero.");
		return false;
	}

	if (MaximumTerrainDepthMetres >= RadiusMetres)
	{
		OutFailureReason = TEXT("MaximumTerrainDepthMetres must be smaller than RadiusMetres.");
		return false;
	}

	return true;
}

FVoraxiaPlanetRuntimeState UVoraxiaPlanetDefinition::CreateRuntimeState() const
{
	FVoraxiaPlanetRuntimeState RuntimeState;

	RuntimeState.PlanetId = PlanetId;
	RuntimeState.DefinitionName = GetFName();
	RuntimeState.Seed = Seed;
	RuntimeState.GeneratorVersion = GeneratorVersion;
	RuntimeState.FeatureProfileId = IsValid(FeatureProfile)
		? FeatureProfile->GetPrimaryAssetId()
		: FPrimaryAssetId();
	RuntimeState.FeatureProfileVersion = IsValid(FeatureProfile)
		? FeatureProfile->ProfileVersion
		: 0;

	if (IsValid(FeatureProfile))
	{
		RuntimeState.ImpactCratering = FeatureProfile->ImpactCratering;
		RuntimeState.Tectonics = FeatureProfile->Tectonics;
		RuntimeState.Volcanism = FeatureProfile->Volcanism;
		RuntimeState.Hydrology = FeatureProfile->Hydrology;
		RuntimeState.Ice = FeatureProfile->Ice;
		RuntimeState.Caves = FeatureProfile->Caves;
		RuntimeState.OreBodies = FeatureProfile->OreBodies;
		RuntimeState.Hazards = FeatureProfile->Hazards;
	}

	RuntimeState.RadiusMetres = RadiusMetres;
	RuntimeState.MaximumTerrainHeightMetres = MaximumTerrainHeightMetres;
	RuntimeState.MaximumTerrainDepthMetres = MaximumTerrainDepthMetres;
	RuntimeState.SurfaceGravityMetresPerSecondSquared = SurfaceGravityMetresPerSecondSquared;

	return RuntimeState;
}
