// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "Camera/CameraShakeBase.h"
#include "VoraxiaCameraSettingsAsset.generated.h"

/**
 * Serializable, authored camera configuration. It stores persistent tuning only:
 * no active focus target, transient runtime blend, active shake instance, or target-camera reference.
 */
USTRUCT(BlueprintType)
struct VORAXIACAMERA_API FVoraxiaCameraPersistentSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Setup")
	bool bAutoFindCameraComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Framing")
	float CameraDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Framing")
	float PivotHeight = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Framing")
	FVector AdditionalCameraOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Framing|Shoulder", meta=(ClampMin="0.0", UIMin="0.0"))
	float ShoulderOffset = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Framing|Shoulder")
	bool bUseRightShoulder = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Framing")
	FVector AdditionalPivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Framing")
	bool bClampCameraOffsetWithinDistance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|FOV")
	bool bUseTargetCameraFOVAsBase = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|FOV")
	float BaseFOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|FOV")
	float MinFOV = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|FOV")
	float MaxFOV = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	float InitialPitch = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	float MinPitch = -55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	float MaxPitch = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	float YawInputSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	float PitchInputSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	bool bInvertPitchInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	bool bEnableRotationLag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	float RotationLagSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation")
	float LookInputDeadZone = 0.01f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Constraints")
	bool bEnableYawConstraints = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Constraints", meta=(EditCondition="bEnableYawConstraints", ClampMin="0.0", UIMin="0.0"))
	float YawConstraintSoftZone = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Constraints|Debug", meta=(EditCondition="bEnableYawConstraints"))
	bool bDrawYawConstraintDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Constraints|Debug", meta=(EditCondition="bEnableYawConstraints && bDrawYawConstraintDebug", ClampMin="10.0", UIMin="10.0"))
	float YawConstraintDebugDrawLength = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision")
	bool bEnableCameraCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision")
	TEnumAsByte<ECollisionChannel> CameraCollisionChannel = ECC_Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision")
	float CameraCollisionRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision")
	float MinDistanceFromPivot = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision")
	float CollisionProbeStartOffset = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision")
	float CollisionSafetyPadding = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision")
	float CollisionCompressionSpeed = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision")
	float CollisionRecoverySpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision|Predictive Avoidance")
	bool bEnablePredictiveAvoidance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision|Predictive Avoidance")
	float FeelerYawOffset = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision|Predictive Avoidance")
	int32 FeelerPairs = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Collision|Predictive Avoidance")
	float FeelerProbeRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	bool bEnableMovementAnticipation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float MovementAnticipationForwardOffset = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float MovementAnticipationBackwardOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float MovementAnticipationSideOffset = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float MovementAnticipationFullSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float MovementAnticipationInterpSpeed = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float MovementAnticipationReturnSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float MovementAnticipationDeadZone = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	bool bReduceMovementAnticipationWhileLooking = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float LookInputAnticipationMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Movement Anticipation")
	float MovementAnticipationLookInputThreshold = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	bool bEnablePitchDistanceModifier = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	float PitchDistanceAtMinPitchOffset = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	float PitchDistanceAtMaxPitchOffset = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	bool bEnablePitchFOVModifier = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	float PitchFOVAtMinPitchOffset = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	float PitchFOVAtMaxPitchOffset = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	float PitchModifierInterpSpeed = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	bool bEnableSpeedDistanceModifier = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	float SpeedDistanceOffset = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	bool bEnableSpeedFOVModifier = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	float SpeedFOVOffset = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	float SpeedModifierFullSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	float SpeedModifierInterpSpeed = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	bool bEnableFocusSystem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	FName DefaultFocusActorTag = TEXT("CameraFocusTarget");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	float DefaultFocusBlendInTime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	float DefaultFocusBlendOutTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	bool bClampFocusPitchToCameraLimits = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	float FocusMinimumTargetDistance = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	float FocusTargetSearchDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	bool bRequireFocusTargetInFront = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	float FocusTargetMinForwardDot = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	bool bRequireFocusTargetLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	TEnumAsByte<ECollisionChannel> FocusTargetLineOfSightChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	float FocusTargetAlignmentScoreWeight = 1000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Focus")
	float FocusTargetDistanceScoreWeight = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug|Focus Selection")
	bool bLogFocusTargetSelection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug|Focus Selection")
	bool bDrawFocusTargetSelectionDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug|Focus Selection")
	float FocusTargetSelectionDebugDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug")
	bool bDrawFocusDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug")
	bool bDrawCameraCollisionDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug")
	bool bDrawCameraStateDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug|Slate")
	bool bShowSlateDebugPanel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug|Slate")
	float SlateDebugPanelWidth = 380.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Debug|Slate")
	int32 SlateDebugPanelZOrder = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Pitch Constraints")
	bool bEnablePitchConstraints = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Pitch Constraints")
	float PitchConstraintTolerance = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Pitch Follow")
	bool bEnablePitchMovementFollow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Pitch Follow")
	float RestingCameraPitch = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Pitch Follow")
	float PitchFollowSpeed = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Pitch Follow")
	float PitchFollowTimeThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Pitch Follow")
	float PitchFollowAngleThreshold = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Pitch Follow")
	float PitchFollowMinSpeedThreshold = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Follow")
	bool bEnableYawMovementFollow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Follow")
	float YawFollowSpeed = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Follow")
	float YawFollowTimeThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Follow")
	float YawFollowAngleThreshold = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Follow")
	float YawFollowMinSpeedThreshold = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Rotation|Yaw Follow")
	bool bYawFollowOnlyForwardMovement = true;

	/** Enables an automatic, subtle local-space shake while the owner is essentially stationary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Idle")
	bool bEnableIdleCameraShake = false;

	/** Shake class played while the owner is idle. Leave unset to disable the idle shake even when enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Idle", meta=(EditCondition="bEnableIdleCameraShake"))
	TSubclassOf<UCameraShakeBase> IdleCameraShakeClass;

	/** Intensity multiplier used by the automatic idle shake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Idle", meta=(EditCondition="bEnableIdleCameraShake", ClampMin="0.0"))
	float IdleCameraShakeScale = 0.10f;

	/** Maximum 2D owner speed that still counts as idle for the idle shake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Idle", meta=(EditCondition="bEnableIdleCameraShake", ClampMin="0.0"))
	float IdleCameraShakeMaxSpeed = 5.0f;

	/** Enables an automatic local-space shake whose intensity follows owner movement speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Movement")
	bool bEnableMovementCameraShake = false;

	/** Shake class played while the owner is moving. Leave unset to disable movement shake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Movement", meta=(EditCondition="bEnableMovementCameraShake"))
	TSubclassOf<UCameraShakeBase> MovementCameraShakeClass;

	/** Owner speed at which the movement shake first becomes active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Movement", meta=(EditCondition="bEnableMovementCameraShake", ClampMin="0.0"))
	float MovementCameraShakeMinSpeed = 150.0f;

	/** Owner speed that maps to the maximum movement shake scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Movement", meta=(EditCondition="bEnableMovementCameraShake", ClampMin="0.0"))
	float MovementCameraShakeMaxSpeed = 850.0f;

	/** Lowest scale used once the movement shake begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Movement", meta=(EditCondition="bEnableMovementCameraShake", ClampMin="0.0"))
	float MovementCameraShakeMinScale = 0.10f;

	/** Highest scale used at or above MovementCameraShakeMaxSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Movement", meta=(EditCondition="bEnableMovementCameraShake", ClampMin="0.0"))
	float MovementCameraShakeMaxScale = 0.45f;

	/** How quickly the movement shake scale reacts to speed changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Camera Shake|Movement", meta=(EditCondition="bEnableMovementCameraShake", ClampMin="0.0"))
	float MovementCameraShakeScaleInterpSpeed = 6.0f;

};


USTRUCT(BlueprintType)
struct VORAXIACAMERA_API FVoraxiaCameraOcclusionDitherSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither")
	bool bEnableCameraOcclusionDither = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Setup")
	bool bAutoFindCameraComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Trace")
	TEnumAsByte<ECollisionChannel> DitherTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Trace")
	float DitherProbeRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Trace")
	float DitherTraceInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Trace")
	int32 MaxDitheredComponents = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Material")
	FName DitherScalarParameterName = TEXT("CameraDitherFade");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Material")
	float OccludedDitherFadeValue = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Material")
	float FadeOutInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Material")
	float FadeInInterpSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Material")
	float RestoredStateReleaseDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Debug")
	bool bDrawDitherDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Occlusion Dither|Debug")
	bool bLogDitherChanges = false;
};

/**
 * Reusable camera preset for exploration, combat, sprint, or character-specific tuning.
 * The owning UVoraxiaCameraComponent can capture its persistent configuration into
 * this asset in the editor, apply it at runtime, and optionally apply linked dither settings.
 */
UCLASS(BlueprintType)
class VORAXIACAMERA_API UVoraxiaCameraSettingsAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Increment only when a breaking settings-asset migration is introduced. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Settings Asset")
	int32 SettingsVersion = 1;

	/** Complete persistent configuration for UVoraxiaCameraComponent, including automatic shake settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Settings Asset")
	FVoraxiaCameraPersistentSettings CameraSettings;

	/** When true, the preset also captures and applies a sibling UVoraxiaCameraOcclusionDitherComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Settings Asset")
	bool bIncludeOcclusionDitherSettings = true;

	/** Persistent occlusion-dither tuning captured from the player camera system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voraxia Camera|Settings Asset", meta=(EditCondition="bIncludeOcclusionDitherSettings"))
	FVoraxiaCameraOcclusionDitherSettings OcclusionDitherSettings;
};
