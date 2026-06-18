#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoraxiaCameraComponent.generated.h"

class UCameraComponent;
class UCurveFloat;

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
	
	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Debug")
	void SetSlateDebugPanelVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category="Voraxia Camera|Debug")
	void ToggleSlateDebugPanel();

	UFUNCTION(BlueprintPure, Category="Voraxia Camera|Debug")
	bool IsSlateDebugPanelVisible() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Setup")
	bool bAutoFindCameraComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	float CameraDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	float PivotHeight = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	FVector AdditionalCameraOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	FVector AdditionalPivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Framing")
	bool bClampCameraOffsetWithinDistance = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|FOV")
	bool bUseTargetCameraFOVAsBase = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|FOV")
	float BaseFOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|FOV")
	float MinFOV = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|FOV")
	float MaxFOV = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float InitialPitch = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float MinPitch = -55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float MaxPitch = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float YawInputSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float PitchInputSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	bool bInvertPitchInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	bool bEnableRotationLag = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation", meta=(EditCondition="bEnableRotationLag"))
	float RotationLagSpeed = 18.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float LookInputDeadZone = 0.01f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision")
	bool bEnableCameraCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	TEnumAsByte<ECollisionChannel> CameraCollisionChannel = ECC_Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CameraCollisionRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float MinDistanceFromPivot = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CollisionProbeStartOffset = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CollisionSafetyPadding = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CollisionCompressionSpeed = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision", meta=(EditCondition="bEnableCameraCollision"))
	float CollisionRecoverySpeed = 12.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision|Predictive Avoidance", meta=(EditCondition="bEnableCameraCollision"))
	bool bEnablePredictiveAvoidance = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision|Predictive Avoidance", meta=(EditCondition="bEnablePredictiveAvoidance", ClampMin="0.0"))
	float FeelerYawOffset = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision|Predictive Avoidance", meta=(EditCondition="bEnablePredictiveAvoidance", ClampMin="0", UIMin="0", UIMax="4"))
	int32 FeelerPairs = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Collision|Predictive Avoidance", meta=(EditCondition="bEnablePredictiveAvoidance", ClampMin="1.0"))
	float FeelerProbeRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug")
	bool bDrawCameraCollisionDebug = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug")
	bool bDrawCameraStateDebug = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug|Slate")
	bool bShowSlateDebugPanel = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug|Slate", meta=(EditCondition="bShowSlateDebugPanel"))
	float SlateDebugPanelWidth = 380.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Debug|Slate", meta=(EditCondition="bShowSlateDebugPanel"))
	int32 SlateDebugPanelZOrder = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Constraints")
	bool bEnablePitchConstraints = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Constraints", meta=(EditCondition="bEnablePitchConstraints"))
	float PitchConstraintTolerance = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow")
	bool bEnablePitchMovementFollow = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float RestingCameraPitch = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float PitchFollowSpeed = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float PitchFollowTimeThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float PitchFollowAngleThreshold = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Pitch Follow", meta=(EditCondition="bEnablePitchMovementFollow"))
	float PitchFollowMinSpeedThreshold = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow")
	bool bEnableYawMovementFollow = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	float YawFollowSpeed = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	float YawFollowTimeThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	float YawFollowAngleThreshold = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	float YawFollowMinSpeedThreshold = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation|Yaw Follow", meta=(EditCondition="bEnableYawMovementFollow"))
	bool bYawFollowOnlyForwardMovement = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Voraxia Camera|Setup")
	TObjectPtr<UCameraComponent> TargetCamera = nullptr;

private:
	FVoraxiaCameraRuntimeFloat RuntimeCameraDistance;
	FVoraxiaCameraRuntimeFloat RuntimePivotHeight;
	FVoraxiaCameraRuntimeFloat RuntimeFOVOffset;

	FVoraxiaCameraRuntimeVector RuntimeCameraOffset;
	FVoraxiaCameraRuntimeVector RuntimePivotOffset;

	FRotator DesiredRotation = FRotator::ZeroRotator;
	FRotator SmoothedRotation = FRotator::ZeroRotator;

	float PendingYawInput = 0.0f;
	float PendingPitchInput = 0.0f;
	
	float TimeSinceLastYawInput = 999.0f;
	float TimeSinceLastPitchInput = 999.0f;
	
	float CurrentCollisionDistanceFromPivot = -1.0f;
	bool bWasCameraCollisionBlocked = false;
	
	float LastDesiredDistanceFromPivot = 0.0f;
	float LastEffectiveDistanceFromPivot = 0.0f;
	
	TSharedPtr<SWidget> SlateDebugPanelWidget;

	FVector LastPivotLocation = FVector::ZeroVector;
	FVector LastDesiredCameraLocation = FVector::ZeroVector;
	FVector LastFinalCameraLocation = FVector::ZeroVector;

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
	void DrawCameraStateDebug() const;
	void CreateSlateDebugPanel();
	void DestroySlateDebugPanel();
};