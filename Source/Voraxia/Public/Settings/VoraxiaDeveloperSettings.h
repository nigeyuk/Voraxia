// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "VoraxiaDeveloperSettings.generated.h"

UENUM(BlueprintType)
enum class EVoraxiaDebugLevel : uint8
{
	None UMETA(DisplayName = "None"),
	Error UMETA(DisplayName = "Error"),
	Warning UMETA(DisplayName = "Warning"),
	Info UMETA(DisplayName = "Info"),
	Verbose UMETA(DisplayName = "Verbose"),
	VeryVerbose UMETA(DisplayName = "Very Verbose")
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Voraxia"))
class VORAXIA_API UVoraxiaDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UVoraxiaDeveloperSettings();

	static const UVoraxiaDeveloperSettings* Get();

	// UDeveloperSettings
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif

public:
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Debugging")
	bool bEnableVoraxiaDebugging = true;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Debugging")
	EVoraxiaDebugLevel DebugLevel = EVoraxiaDebugLevel::Info;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Debugging")
	bool bEnableOnScreenDebugMessages = true;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Debugging")
	bool bEnableVoxelDebugging = false;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Debugging")
	bool bEnableCharacterDebugging = false;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Debugging")
	bool bEnableCameraDebugging = false;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Logging")
	bool bLogCharacterLifecycle = true;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Logging")
	bool bLogInputSetup = true;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Logging")
	bool bLogCameraSetup = true;
};
