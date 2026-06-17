// Copyright 2026 Coding Custard Studios.

#include "Settings/VoraxiaDeveloperSettings.h"

#define LOCTEXT_NAMESPACE "VoraxiaDeveloperSettings"

UVoraxiaDeveloperSettings::UVoraxiaDeveloperSettings()
{
}

const UVoraxiaDeveloperSettings* UVoraxiaDeveloperSettings::Get()
{
	return GetDefault<UVoraxiaDeveloperSettings>();
}

FName UVoraxiaDeveloperSettings::GetContainerName() const
{
	return TEXT("Project");
}

FName UVoraxiaDeveloperSettings::GetCategoryName() const
{
	return TEXT("Voraxia");
}

FName UVoraxiaDeveloperSettings::GetSectionName() const
{
	return TEXT("General");
}

#if WITH_EDITOR

FText UVoraxiaDeveloperSettings::GetSectionText() const
{
	return LOCTEXT("VoraxiaSettingsSectionText", "General");
}

FText UVoraxiaDeveloperSettings::GetSectionDescription() const
{
	return LOCTEXT(
		"VoraxiaSettingsSectionDescription",
		"Project-wide settings for Voraxia debugging, logging, and development behaviour."
	);
}

#endif

#undef LOCTEXT_NAMESPACE
