// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoraxiaCameraComponent.generated.h"

class AActor;
class UCameraComponent;
class UCurveFloat;
class SWidget;
class USceneComponent;

struct FVoraxiaCameraRuntimeFloat
{
	float CurrentValue = 0.0f;
	float StartValue = 0.0f;
	float TargetValue = 0.0f;

	float BlendTime = 0.0f;
	float BlendElapsed = 0.0f;

	TWeakObjectPtr<UCurveFloat> BlendCurve;

	void Initialize(float InValue);
	void Set(float InValue, float InBlendTime, UCurveFloat* InBlendCurve);
	float Tick(float DeltaTime);
};

struct FVoraxiaCameraRuntimeVector
{
	FVector CurrentValue = FVector::ZeroVector;
	FVector StartValue = FVector::ZeroVector;
	FVector TargetValue = FVector::ZeroVector;

	float BlendTime = 0.0f;
	float BlendElapsed = 0.0f;

	TWeakObjectPtr<UCurveFloat> BlendCurve;

	void Initialize(const FVector& InValue);
	void Set(const FVector& InValue, float InBlendTime, UCurveFloat* InBlendCurve);
	FVector Tick(float DeltaTime);
};

UCLASS(ClassGroup=(Voraxia), meta=(BlueprintSpawnableComponent))
class VORAXIACAMERA_API UVoraxiaCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoraxiaCameraComponent();

	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;
	


	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Setup")
	void SetTargetCamera(UCameraComponent* InTargetCamera);

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Setup")
	UCameraComponent* GetTargetCamera() const;

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Input")
	void AddYawInput(float Value);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Input")
	void AddPitchInput(float Value);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void SetFraming(
		float NewCameraDistance,
		float NewPivotHeight,
		FVector NewCameraOffset,
		FVector NewPivotOffset,
		float NewFOVOffset,
		float BlendTime = 0.25f,
		UCurveFloat* BlendCurve = nullptr
	);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void ResetFraming(float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void SetCameraDistance(float NewCameraDistance, float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void ResetCameraDistance(float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void SetPivotHeight(float NewPivotHeight, float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void ResetPivotHeight(float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void SetCameraOffset(FVector NewRuntimeCameraOffset, float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void ResetCameraOffset(float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void SetPivotOffset(FVector NewRuntimePivotOffset, float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void ResetPivotOffset(float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void SetFOVOffset(float NewRuntimeFOVOffset, float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void ResetFOVOffset(float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Framing")
	void SwapCameraShoulder(float BlendTime = 0.25f, UCurveFloat* BlendCurve = nullptr);
	
	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Focus")
	void SetFocusActor(AActor* NewFocusActor, float BlendTime = 0.35f, FName SocketName = NAME_None);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Focus")
	void SetFocusComponent(USceneComponent* NewFocusComponent, float BlendTime = 0.35f, FName SocketName = NAME_None);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Focus")
	void SetFocusWorldLocation(FVector NewFocusWorldLocation, float BlendTime = 0.35f);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Focus")
	void ClearFocus(float BlendTime = 0.25f);

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Focus")
	bool IsFocusActive() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Focus")
	float GetCurrentFocusAlpha() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Focus")
	FVector GetCurrentFocusLocation() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Focus")
	bool HasFocusTarget() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Focus")
	AActor* GetCurrentFocusActor() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Focus")
	USceneComponent* GetCurrentFocusComponent() const;
	
	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Focus")
	FString GetCurrentFocusTargetName() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Scan")
	bool IsCurrentFocusTargetScannable() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Scan")
	FText GetCurrentFocusScanDisplayName() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Scan")
	FText GetCurrentFocusScanSummary() const;
	
	/** Finds the first actor with DefaultFocusActorTag and focuses it. */
	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Focus")
	void FocusDefaultTaggedActor(float BlendTime = -1.0f);
	
	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Movement Anticipation")
	FVector GetCurrentMovementAnticipationOffset() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Framing")
	float GetCurrentCameraDistance() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Framing")
	float GetCurrentPivotHeight() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Framing")
	FVector GetCurrentCameraOffset() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Framing")
	FVector GetCurrentPivotOffset() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Framing")
	float GetCurrentFOV() const;
	
	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	bool IsCameraCollisionBlocked() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	float GetDesiredDistanceFromPivot() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	float GetEffectiveDistanceFromPivot() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	float GetCurrentCollisionDistanceFromPivot() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	FVector GetLastPivotLocation() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	FVector GetLastDesiredCameraLocation() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	FVector GetLastFinalCameraLocation() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	FRotator GetDesiredCameraRotation() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	FRotator GetSmoothedCameraRotation() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	FString GetCameraDebugSummary() const;
	
	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Dynamic Modifiers")
	float GetCurrentPitchDistanceOffset() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Dynamic Modifiers")
	float GetCurrentPitchFOVOffset() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Dynamic Modifiers")
	float GetCurrentSpeedDistanceOffset() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Dynamic Modifiers")
	float GetCurrentSpeedFOVOffset() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Dynamic Modifiers")
	float GetCurrentDynamicDistanceOffset() const;

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Dynamic Modifiers")
	float GetCurrentDynamicFOVOffset() const;
	
	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Debug")
	void SetSlateDebugPanelVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Debug")
	void ToggleSlateDebugPanel();

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	bool IsSlateDebugPanelVisible() const;
	

protected:
	/** Automatically finds the first camera component on the owning actor during BeginPlay if no target camera has been set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Setup")
	bool bAutoFindCameraComponent = true;

	/** Default distance, in Unreal units, from the pivot to the camera before dynamic modifiers and collision are applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	float CameraDistance = 450.0f;

	/** Default vertical height, in Unreal units, added to the owning actor location to create the camera pivot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	float PivotHeight = 70.0f;

	/** Permanent local-space offset applied to the camera position after the base distance is calculated. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	FVector AdditionalCameraOffset = FVector::ZeroVector;

	/** Permanent world-space offset added to the camera pivot before the camera position is calculated. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	FVector AdditionalPivotOffset = FVector::ZeroVector;

	/** Keeps the final local camera offset inside the current camera distance so framing offsets cannot push the camera too far out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	bool bClampCameraOffsetWithinDistance = true;

	/** Uses the assigned camera component's current FOV as the base FOV during BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|FOV")
	bool bUseTargetCameraFOVAsBase = true;

	/** Base field of view used before runtime, pitch, and speed FOV offsets are added. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|FOV")
	float BaseFOV = 90.0f;

	/** Minimum final field of view allowed after all FOV modifiers are applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|FOV")
	float MinFOV = 30.0f;

	/** Maximum final field of view allowed after all FOV modifiers are applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|FOV")
	float MaxFOV = 120.0f;

	/** Starting camera pitch, in degrees, used when the component begins play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float InitialPitch = -10.0f;

	/** Lowest allowed camera pitch, in degrees, after pitch limits and focus pitch clamping are applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float MinPitch = -55.0f;

	/** Highest allowed camera pitch, in degrees, after pitch limits and focus pitch clamping are applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float MaxPitch = 25.0f;

	/** Scales horizontal look input into yaw rotation speed in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float YawInputSpeed = 180.0f;

	/** Scales vertical look input into pitch rotation speed in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float PitchInputSpeed = 120.0f;

	/** Inverts vertical look input before it is applied to the camera pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	bool bInvertPitchInput = false;

	/** Smooths the camera rotation toward the desired rotation instead of snapping instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	bool bEnableRotationLag = true;

	/** How quickly the smoothed camera rotation catches up to the desired rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation", meta=(EditCondition="bEnableRotationLag"))
	float RotationLagSpeed = 18.0f;
	
	/** Small input threshold used to ignore tiny yaw or pitch input noise. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float LookInputDeadZone = 0.01f;
	
	/** Enables camera collision sweeps between the pivot and desired camera location. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision")
	bool bEnableCameraCollision = true;

	/** Collision channel used by the main camera collision probe. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	TEnumAsByte<ECollisionChannel> CameraCollisionChannel = ECC_Camera;

	/** Radius, in Unreal units, of the main sphere sweep used for camera collision. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CameraCollisionRadius = 12.0f;

	/** Closest distance, in Unreal units, the camera is allowed to move toward the pivot during collision compression. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float MinDistanceFromPivot = 80.0f;

	/** Distance, in Unreal units, from the pivot where collision sweeps begin. Lower values help tight wall cases. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CollisionProbeStartOffset = 12.0f;

	/** Extra distance, in Unreal units, kept between the camera and a collision hit point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CollisionSafetyPadding = 4.0f;

	/** How quickly the camera moves inward when collision blocks the desired position. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CollisionCompressionSpeed = 35.0f;

	/** How quickly the camera moves back outward after collision clears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CollisionRecoverySpeed = 12.0f;
	
	/** Enables extra side feeler probes that anticipate nearby walls before the main camera path hits them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision|Predictive Avoidance", meta=(EditCondition="bEnableCameraCollision"))
	bool bEnablePredictiveAvoidance = true;

	/** Yaw angle, in degrees, between each predictive avoidance feeler pair. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision|Predictive Avoidance", meta=(EditCondition="bEnablePredictiveAvoidance", ClampMin="0.0"))
	float FeelerYawOffset = 18.0f;

	/** Number of left and right predictive avoidance feeler pairs to trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision|Predictive Avoidance", meta=(EditCondition="bEnablePredictiveAvoidance", ClampMin="0", UIMin="0", UIMax="4"))
	int32 FeelerPairs = 2;

	/** Radius, in Unreal units, used for predictive avoidance feeler sweeps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision|Predictive Avoidance", meta=(EditCondition="bEnablePredictiveAvoidance", ClampMin="1.0"))
	float FeelerProbeRadius = 8.0f;
	
	/** Enables subtle camera offset based on the owning actor's movement direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation")
	bool bEnableMovementAnticipation = true;

	/** Maximum local camera offset applied when moving forward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bEnableMovementAnticipation"))
	float MovementAnticipationForwardOffset = 20.0f;

	/** Maximum local camera offset applied when moving backward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bEnableMovementAnticipation"))
	float MovementAnticipationBackwardOffset = 10.0f;

	/** Maximum local camera offset applied when strafing left or right. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bEnableMovementAnticipation"))
	float MovementAnticipationSideOffset = 35.0f;

	/** Movement speed that produces full movement anticipation strength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bEnableMovementAnticipation"))
	float MovementAnticipationFullSpeed = 500.0f;

	/** How quickly movement anticipation blends toward its target offset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bEnableMovementAnticipation"))
	float MovementAnticipationInterpSpeed = 4.0f;

	/** How quickly movement anticipation returns to neutral when movement stops. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bEnableMovementAnticipation"))
	float MovementAnticipationReturnSpeed = 6.0f;

	/** Minimum normalized movement speed before movement anticipation begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bEnableMovementAnticipation"))
	float MovementAnticipationDeadZone = 0.05f;

	/** Reduces movement anticipation while the player is actively moving the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bEnableMovementAnticipation"))
	bool bReduceMovementAnticipationWhileLooking = true;

	/** Multiplier applied to movement anticipation while recent look input is detected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bReduceMovementAnticipationWhileLooking"))
	float LookInputAnticipationMultiplier = 0.35f;

	/** Time window, in seconds, during which recent look input suppresses movement anticipation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Movement Anticipation", meta=(EditCondition="bReduceMovementAnticipationWhileLooking"))
	float MovementAnticipationLookInputThreshold = 0.2f;
	
	/** Allows camera pitch to add or remove camera distance dynamically. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	bool bEnablePitchDistanceModifier = true;

	/** Distance offset applied when the camera reaches the minimum pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Pitch", meta=(EditCondition="bEnablePitchDistanceModifier"))
	float PitchDistanceAtMinPitchOffset = 35.0f;

	/** Distance offset applied when the camera reaches the maximum pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Pitch", meta=(EditCondition="bEnablePitchDistanceModifier"))
	float PitchDistanceAtMaxPitchOffset = 15.0f;

	/** Allows camera pitch to adjust field of view dynamically. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	bool bEnablePitchFOVModifier = true;

	/** FOV offset applied when the camera reaches the minimum pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Pitch", meta=(EditCondition="bEnablePitchFOVModifier"))
	float PitchFOVAtMinPitchOffset = 3.0f;

	/** FOV offset applied when the camera reaches the maximum pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Pitch", meta=(EditCondition="bEnablePitchFOVModifier"))
	float PitchFOVAtMaxPitchOffset = 1.0f;

	/** How quickly pitch-based distance and FOV modifiers blend toward their targets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Pitch")
	float PitchModifierInterpSpeed = 4.0f;

	/** Allows owner movement speed to add extra camera distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	bool bEnableSpeedDistanceModifier = true;

	/** Maximum distance offset added at full speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Speed", meta=(EditCondition="bEnableSpeedDistanceModifier"))
	float SpeedDistanceOffset = 35.0f;

	/** Allows owner movement speed to add extra field of view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	bool bEnableSpeedFOVModifier = true;

	/** Maximum FOV offset added at full speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Speed", meta=(EditCondition="bEnableSpeedFOVModifier"))
	float SpeedFOVOffset = 5.0f;

	/** Movement speed that produces full speed-based camera modifier strength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	float SpeedModifierFullSpeed = 600.0f;

	/** How quickly speed-based distance and FOV modifiers blend toward their targets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Dynamic Modifiers|Speed")
	float SpeedModifierInterpSpeed = 4.0f;
	
	/** Enables rotation-only camera focus toward actors, components, sockets, or world locations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus")
	bool bEnableFocusSystem = true;

	/** Actor tag used when FocusDefaultTaggedActor searches the level for a focus target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus")
	FName DefaultFocusActorTag = TEXT("CameraFocusTarget");
	
	/** Default time, in seconds, used when blending into focus if no explicit blend time is supplied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus", meta=(EditCondition="bEnableFocusSystem"))
	float DefaultFocusBlendInTime = 0.35f;

	/** Default time, in seconds, used when clearing focus if no explicit blend time is supplied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus", meta=(EditCondition="bEnableFocusSystem"))
	float DefaultFocusBlendOutTime = 0.25f;

	/** Clamps focus-driven pitch to the normal camera pitch limits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus", meta=(EditCondition="bEnableFocusSystem"))
	bool bClampFocusPitchToCameraLimits = true;

	/** Minimum distance, in Unreal units, required before the camera attempts to rotate toward a focus target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus", meta=(EditCondition="bEnableFocusSystem"))
	float FocusMinimumTargetDistance = 25.0f;
	
	/** Maximum distance, in Unreal units, used when searching for tagged focus targets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus", meta=(EditCondition="bEnableFocusSystem", ClampMin="0.0"))
	float FocusTargetSearchDistance = 5000.0f;

	/** Ignores tagged focus targets that are behind the current camera view direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus", meta=(EditCondition="bEnableFocusSystem"))
	bool bRequireFocusTargetInFront = true;

	/** Minimum forward dot value required when bRequireFocusTargetInFront is enabled. Higher means target must be closer to the centre of view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Focus", meta=(EditCondition="bEnableFocusSystem", ClampMin="-1.0", ClampMax="1.0"))
	float FocusTargetMinForwardDot = 0.15f;
	
	/** Draws the active focus target, focus line, and focus point while the focus system is active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug")
	bool bDrawFocusDebug = false;
	
	/** Draws collision and predictive avoidance probe lines and hit markers in the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug")
	bool bDrawCameraCollisionDebug = false;
	
	/** Draws pivot, desired camera, final camera, and rotation debug information in the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug")
	bool bDrawCameraStateDebug = false;
	
	/** Shows the on-screen Slate debug panel when the component begins play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug|Slate")
	bool bShowSlateDebugPanel = false;

	/** Width, in pixels, of the on-screen Slate camera debug panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug|Slate", meta=(EditCondition="bShowSlateDebugPanel"))
	float SlateDebugPanelWidth = 380.0f;

	/** Viewport Z-order used when adding the Slate debug panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug|Slate", meta=(EditCondition="bShowSlateDebugPanel"))
	int32 SlateDebugPanelZOrder = 50;

	/** Enables soft clamping near the minimum and maximum camera pitch limits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Constraints")
	bool bEnablePitchConstraints = true;

	/** Soft zone, in degrees, near pitch limits where pitch input is gradually reduced. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Constraints", meta=(EditCondition="bEnablePitchConstraints"))
	float PitchConstraintTolerance = 8.0f;

	/** Allows the camera pitch to ease back toward a resting pitch while the owner is moving. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow")
	bool bEnablePitchMovementFollow = true;

	/** Pitch angle, in degrees, that pitch follow returns to when movement follow is active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float RestingCameraPitch = -10.0f;

	/** How quickly pitch follow returns the camera toward the resting pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float PitchFollowSpeed = 3.5f;

	/** Time, in seconds, after pitch input before pitch follow is allowed to resume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float PitchFollowTimeThreshold = 0.75f;

	/** Minimum pitch difference, in degrees, required before pitch follow makes an adjustment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float PitchFollowAngleThreshold = 3.0f;

	/** Minimum owner movement speed required before pitch follow can activate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float PitchFollowMinSpeedThreshold = 50.0f;

	/** Allows the camera yaw to follow the owner's movement direction when there is no recent yaw input. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow")
	bool bEnableYawMovementFollow = false;

	/** How quickly yaw follow turns the desired camera yaw toward movement direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	float YawFollowSpeed = 2.5f;

	/** Time, in seconds, after yaw input before yaw follow is allowed to resume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	float YawFollowTimeThreshold = 1.0f;

	/** Minimum yaw difference, in degrees, required before yaw follow makes an adjustment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	float YawFollowAngleThreshold = 15.0f;

	/** Minimum owner movement speed required before yaw follow can activate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	float YawFollowMinSpeedThreshold = 150.0f;

	/** Prevents yaw follow from reacting to backpedal or mostly sideways movement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	bool bYawFollowOnlyForwardMovement = true;

	/** Camera component controlled by this Voraxia camera component. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Voraxia Camera|Setup")
	TObjectPtr<UCameraComponent> TargetCamera = nullptr;

private:
	FVoraxiaCameraRuntimeFloat RuntimeCameraDistance;
	FVoraxiaCameraRuntimeFloat RuntimePivotHeight;
	FVoraxiaCameraRuntimeFloat RuntimeFOVOffset;

	FVoraxiaCameraRuntimeVector RuntimeCameraOffset;
	FVoraxiaCameraRuntimeVector RuntimePivotOffset;

	FRotator DesiredRotation = FRotator::ZeroRotator;
	FRotator UnfocusedDesiredRotation = FRotator::ZeroRotator;
	FRotator SmoothedRotation = FRotator::ZeroRotator;

	float PendingYawInput = 0.0f;
	float PendingPitchInput = 0.0f;
	
	float TimeSinceLastYawInput = 999.0f;
	float TimeSinceLastPitchInput = 999.0f;
	
	float CurrentCollisionDistanceFromPivot = -1.0f;
	bool bWasCameraCollisionBlocked = false;
	
	float LastDesiredDistanceFromPivot = 0.0f;
	float LastEffectiveDistanceFromPivot = 0.0f;
	
	float CurrentPitchDistanceOffset = 0.0f;
	float CurrentPitchFOVOffset = 0.0f;

	float CurrentSpeedDistanceOffset = 0.0f;
	float CurrentSpeedFOVOffset = 0.0f;
	
	TWeakObjectPtr<AActor> FocusTargetActor;
	TWeakObjectPtr<USceneComponent> FocusTargetComponent;

	FName FocusTargetSocketName = NAME_None;

	FVector FocusWorldLocation = FVector::ZeroVector;
	FVector CachedFocusLocation = FVector::ZeroVector;

	bool bFocusUsesWorldLocation = false;
	bool bFocusActive = false;
	bool bHasCachedFocusLocation = false;

	float FocusAlpha = 0.0f;
	float FocusBlendStartAlpha = 0.0f;
	float FocusBlendTargetAlpha = 0.0f;
	float FocusBlendTime = 0.0f;
	float FocusBlendElapsed = 0.0f;
	
	TSharedPtr<SWidget> SlateDebugPanelWidget;

	FVector LastPivotLocation = FVector::ZeroVector;
	FVector LastDesiredCameraLocation = FVector::ZeroVector;
	FVector LastFinalCameraLocation = FVector::ZeroVector;
	FVector CurrentMovementAnticipationOffset = FVector::ZeroVector;

	void InitializeRuntimeState();
	void UpdateRuntimeState(float DeltaTime);
	void UpdateInputRotation(float DeltaTime);

	FVector CalculatePivotLocation() const;
	FTransform CalculateCameraTransform(float DeltaTime);
	FVector ResolveCameraCollision(const FVector& PivotLocation, const FVector& DesiredCameraLocation, float DeltaTime);
	
	bool SweepCameraCollisionProbe(
		const FVector& PivotLocation,
		const FVector& DirectionFromPivot,
		float DesiredDistanceFromPivot,
		float ProbeRadius,
		float& OutSafeDistanceFromPivot,
		FHitResult& OutHit
	) const;
	
	void ApplyCameraTransform(float DeltaTime);
	float CalculateFinalFOV() const;
	
	void ApplyPitchInputWithConstraints(float PitchInputDelta);
	
	bool HasRecentYawInput() const;
	bool HasRecentPitchInput() const;
	
	float GetOwnerMovementSpeed2D() const;
	float GetOwnerMovementYaw() const;
	
	void UpdatePitchFollow(float DeltaTime);
	void UpdateYawFollow(float DeltaTime);
	
	void UpdateMovementAnticipation(float DeltaTime);
	FVector CalculateMovementAnticipationTarget() const;
	
	void UpdateDynamicModifiers(float DeltaTime);

	float CalculatePitchDistanceOffsetTarget() const;
	float CalculatePitchFOVOffsetTarget() const;

	float CalculateSpeedDistanceOffsetTarget() const;
	float CalculateSpeedFOVOffsetTarget() const;
	float CalculateSpeedModifierAlpha() const;
	
	void DrawCameraStateDebug() const;
	void CreateSlateDebugPanel();
	void DestroySlateDebugPanel();
	
	void LogScannableFocusTarget(AActor* Actor) const;
	void UpdateFocus(float DeltaTime);
	void BeginFocusBlend(float TargetAlpha, float BlendTime);
	bool TryGetFocusLocation(FVector& OutFocusLocation) const;
	void ResetFocusTarget();
};