// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetFeatureProfile.cpp
 * @brief Implementation of the Voraxia planet feature profile data asset.
 */

#include "Planet/VoraxiaPlanetFeatureProfile.h"

const FPrimaryAssetType UVoraxiaPlanetFeatureProfile::PrimaryAssetType(
	TEXT("VoraxiaPlanetFeatureProfile"));

FPrimaryAssetId UVoraxiaPlanetFeatureProfile::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(
		PrimaryAssetType,
		GetFName());
}

bool UVoraxiaPlanetFeatureProfile::IsProfileValid(
	FString& OutFailureReason) const
{
	OutFailureReason.Reset();

	if (ProfileDisplayName.IsEmpty())
	{
		OutFailureReason =
			TEXT("ProfileDisplayName must not be empty.");

		return false;
	}

	if (ProfileVersion <= 0)
	{
		OutFailureReason =
			TEXT("ProfileVersion must be greater than zero.");

		return false;
	}

	const FVoraxiaPlanetFeatureControl* FeatureControls[] =
	{
		&ImpactCratering,
		&Tectonics,
		&Volcanism,
		&Hydrology,
		&Ice,
		&Caves,
		&OreBodies,
		&Hazards
	};

	for (const FVoraxiaPlanetFeatureControl* FeatureControl
		: FeatureControls)
	{
		if (FeatureControl == nullptr)
		{
			OutFailureReason =
				TEXT("A planet feature control pointer was unexpectedly null.");

			return false;
		}

		if (!FMath::IsFinite(FeatureControl->Intensity)
			|| !FMath::IsFinite(FeatureControl->Frequency))
		{
			OutFailureReason =
				TEXT("Feature intensity and frequency values must be finite.");

			return false;
		}

		if (FeatureControl->Intensity < 0.0
			|| FeatureControl->Frequency < 0.0)
		{
			OutFailureReason =
				TEXT("Feature intensity and frequency values cannot be negative.");

			return false;
		}
	}

	return true;
}