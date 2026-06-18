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
	float MinPitch = -80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia Camera|Rotation")
	float MaxPitch = 60.0f;

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

	void InitializeRuntimeState();
	void UpdateRuntimeState(float DeltaTime);
	void UpdateInputRotation(float DeltaTime);

	FVector CalculatePivotLocation() const;
	FTransform CalculateCameraTransform() const;

	void ApplyCameraTransform();
	float CalculateFinalFOV() const;
};