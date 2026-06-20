// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoraxiaCameraOcclusionDitherComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPrimitiveComponent;
class UVoraxiaCameraComponent;
struct FVoraxiaCameraOcclusionDitherSettings;

/**
 * Fades dither-ready mesh materials when their components sit between the
 * Voraxia camera pivot and the final camera position.
 *
 * Materials must expose the configured scalar parameter, CameraDitherFade by
 * default, and use it in a dither-aware opacity-mask path.
 */
UCLASS(ClassGroup=(Voraxia), meta=(BlueprintSpawnableComponent))
class VORAXIACAMERA_API UVoraxiaCameraOcclusionDitherComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoraxiaCameraOcclusionDitherComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	/** Assigns the camera component whose pivot and final camera locations drive occlusion testing. */
	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Occlusion Dither")
	void SetCameraComponent(UVoraxiaCameraComponent* InCameraComponent);

	/** Returns the camera component currently used by the occlusion dither system. */
	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Occlusion Dither")
	UVoraxiaCameraComponent* GetCameraComponent() const;

	/** Number of components currently being faded or recovering from a dither fade. */
	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Occlusion Dither")
	int32 GetTrackedDitheredComponentCount() const;

	/** Number of components currently detected between the pivot and final camera position. */
	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Occlusion Dither")
	int32 GetActiveOccludingComponentCount() const;


	/** Copies every persistent occlusion-dither tuning value into a settings struct. */
	void CaptureSettings(FVoraxiaCameraOcclusionDitherSettings& OutSettings) const;

	/** Replaces persistent occlusion-dither tuning from a settings struct and safely clears cached dynamic materials. */
	void ApplySettings(const FVoraxiaCameraOcclusionDitherSettings& InSettings);

protected:
	/** Enables camera-to-character occlusion dithering. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither")
	bool bEnableCameraOcclusionDither = true;

	/** Finds UVoraxiaCameraComponent on the owner during BeginPlay when none has been assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Setup")
	bool bAutoFindCameraComponent = true;

	/** Collision channel used by the pivot-to-camera occlusion sweep. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Trace")
	TEnumAsByte<ECollisionChannel> DitherTraceChannel = ECC_Visibility;

	/** Radius, in Unreal units, of the sphere sweep used to detect visual occluders. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Trace", meta=(ClampMin="0.0"))
	float DitherProbeRadius = 18.0f;

	/** Minimum time, in seconds, between occluder sweeps. Interpolation still runs every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Trace", meta=(ClampMin="0.0"))
	float DitherTraceInterval = 0.05f;

	/** Maximum number of unique primitive components that may be faded at once. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Trace", meta=(ClampMin="1", ClampMax="16"))
	int32 MaxDitheredComponents = 4;

	/** Scalar parameter that dither-ready materials expose for runtime fading. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Material")
	FName DitherScalarParameterName = TEXT("CameraDitherFade");

	/** Scalar value applied while a component obstructs the camera's view of the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Material", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OccludedDitherFadeValue = 0.20f;

	/** How quickly a newly detected occluder fades toward OccludedDitherFadeValue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Material", meta=(ClampMin="0.0"))
	float FadeOutInterpSpeed = 12.0f;

	/** How quickly a cleared occluder restores to its normal material value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Material", meta=(ClampMin="0.0"))
	float FadeInInterpSpeed = 6.0f;

	/** Time, in seconds, a fully restored component keeps cached material instances before its original materials are restored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Material", meta=(ClampMin="0.0"))
	float RestoredStateReleaseDelay = 2.0f;

	/** Draws the pivot-to-camera test path and detected dither components. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Debug")
	bool bDrawDitherDebug = false;

	/** Writes concise start/stop messages when components enter or leave dither state. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Occlusion Dither|Debug")
	bool bLogDitherChanges = false;

private:
	struct FMaterialDitherSlot
	{
		int32 MaterialIndex = INDEX_NONE;
		TObjectPtr<UMaterialInterface> OriginalMaterial;
		TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
	};

	struct FTrackedDitherComponent
	{
		TWeakObjectPtr<UPrimitiveComponent> Component;
		TArray<FMaterialDitherSlot> MaterialSlots;

		float CurrentFade = 1.0f;
		float TimeFullyRestored = 0.0f;
		bool bShouldDither = false;
		bool bWasDitheringLastUpdate = false;
	};

	TWeakObjectPtr<UVoraxiaCameraComponent> CameraComponent;
	TArray<FTrackedDitherComponent> TrackedComponents;

	float TraceTimeAccumulator = 0.0f;
	int32 ActiveOccludingComponentCount = 0;

	void RefreshOccludingComponents();
	void UpdateTrackedComponentFades(float DeltaTime);

	bool EnsureTrackedDitherComponent(UPrimitiveComponent* Component);
	bool DoesMaterialSupportDitherParameter(const UMaterialInterface* Material) const;

	FTrackedDitherComponent* FindTrackedComponent(UPrimitiveComponent* Component);
	const FTrackedDitherComponent* FindTrackedComponent(const UPrimitiveComponent* Component) const;

	void ApplyFadeToTrackedComponent(FTrackedDitherComponent& TrackedComponent, float FadeValue) const;
	void RestoreOriginalMaterials(FTrackedDitherComponent& TrackedComponent) const;
	void RestoreAllOriginalMaterials();

	void DrawDitherTraceDebug(
		const FVector& PivotLocation,
		const FVector& FinalCameraLocation,
		const TArray<FHitResult>& Hits
	) const;
};
