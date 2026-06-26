// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "VoraxiaPlanetDeveloperSettings.generated.h"

/**
 * Project-wide defaults and diagnostics for VoraxiaPlanet.
 *
 * This is deliberately tiny for now. Planet definitions, seeds, terrain
 * settings, and generation profiles will live in dedicated assets later.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Voraxia Planet"))
class VORAXIAPLANET_API UVoraxiaPlanetDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	/**
	 * Enables planet-specific visual and runtime debug tools when they exist.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	bool bEnablePlanetDebugging = false;

	/**
	 * Allows the plugin to write useful lifecycle messages through VoraxiaLog
	 * once the first runtime planet actor exists.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	bool bLogPlanetLifecycle = true;

	/**
	 * Enables diagnostics for chunk relevance, revisions, prediction,
	 * reconciliation, and late-join resync once networking is implemented.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Networking")
	bool bEnableNetworkDiagnostics = false;
};