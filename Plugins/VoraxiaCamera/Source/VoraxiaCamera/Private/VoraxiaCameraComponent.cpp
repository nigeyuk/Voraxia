#include "VoraxiaCameraComponent.h"

#include "Camera/CameraComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"
#include "VoraxiaCameraLog.h"

void FVoraxiaCameraRuntimeFloat::Initialize(const float InValue)
{
	CurrentValue = InValue;
	StartValue = InValue;
	TargetValue = InValue;
	BlendTime = 0.0f;
	BlendElapsed = 0.0f;
	BlendCurve = nullptr;
}

void FVoraxiaCameraRuntimeFloat::Set(
	const float InValue,
	const float InBlendTime,
	UCurveFloat* InBlendCurve
)
{
	StartValue = CurrentValue;
	TargetValue = InValue;
	BlendTime = FMath::Max(0.0f, InBlendTime);
	BlendElapsed = 0.0f;
	BlendCurve = InBlendCurve;

	if (BlendTime <= KINDA_SMALL_NUMBER)
	{
		CurrentValue = TargetValue;
		BlendTime = 0.0f;
		BlendElapsed = 0.0f;
		BlendCurve = nullptr;
	}
}

float FVoraxiaCameraRuntimeFloat::Tick(const float DeltaTime)
{
	if (BlendTime <= KINDA_SMALL_NUMBER)
	{
		CurrentValue = TargetValue;
		return CurrentValue;
	}

	BlendElapsed += DeltaTime;

	float Alpha = FMath::Clamp(BlendElapsed / BlendTime, 0.0f, 1.0f);

	if (const UCurveFloat* Curve = BlendCurve.Get())
	{
		Alpha = Curve->GetFloatValue(Alpha);
	}
	else
	{
		Alpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	}

	CurrentValue = FMath::Lerp(StartValue, TargetValue, Alpha);

	if (BlendElapsed >= BlendTime)
	{
		CurrentValue = TargetValue;
		BlendTime = 0.0f;
		BlendElapsed = 0.0f;
		BlendCurve = nullptr;
	}

	return CurrentValue;
}

void FVoraxiaCameraRuntimeVector::Initialize(const FVector& InValue)
{
	CurrentValue = InValue;
	StartValue = InValue;
	TargetValue = InValue;
	BlendTime = 0.0f;
	BlendElapsed = 0.0f;
	BlendCurve = nullptr;
}

void FVoraxiaCameraRuntimeVector::Set(
	const FVector& InValue,
	const float InBlendTime,
	UCurveFloat* InBlendCurve
)
{
	StartValue = CurrentValue;
	TargetValue = InValue;
	BlendTime = FMath::Max(0.0f, InBlendTime);
	BlendElapsed = 0.0f;
	BlendCurve = InBlendCurve;

	if (BlendTime <= KINDA_SMALL_NUMBER)
	{
		CurrentValue = TargetValue;
		BlendTime = 0.0f;
		BlendElapsed = 0.0f;
		BlendCurve = nullptr;
	}
}

FVector FVoraxiaCameraRuntimeVector::Tick(const float DeltaTime)
{
	if (BlendTime <= KINDA_SMALL_NUMBER)
	{
		CurrentValue = TargetValue;
		return CurrentValue;
	}

	BlendElapsed += DeltaTime;

	float Alpha = FMath::Clamp(BlendElapsed / BlendTime, 0.0f, 1.0f);

	if (const UCurveFloat* Curve = BlendCurve.Get())
	{
		Alpha = Curve->GetFloatValue(Alpha);
	}
	else
	{
		Alpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	}

	CurrentValue = FMath::Lerp(StartValue, TargetValue, Alpha);

	if (BlendElapsed >= BlendTime)
	{
		CurrentValue = TargetValue;
		BlendTime = 0.0f;
		BlendElapsed = 0.0f;
		BlendCurve = nullptr;
	}

	return CurrentValue;
}

UVoraxiaCameraComponent::UVoraxiaCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UVoraxiaCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!TargetCamera && bAutoFindCameraComponent)
	{
		if (AActor* Owner = GetOwner())
		{
			TargetCamera = Owner->FindComponentByClass<UCameraComponent>();
		}
	}

	if (TargetCamera && bUseTargetCameraFOVAsBase)
	{
		BaseFOV = TargetCamera->FieldOfView;
	}

	InitializeRuntimeState();

	const FRotator OwnerRotation = GetOwner() ? GetOwner()->GetActorRotation() : FRotator::ZeroRotator;

	DesiredRotation = FRotator(
		FMath::Clamp(InitialPitch, MinPitch, MaxPitch),
		OwnerRotation.Yaw,
		0.0f
	);

	SmoothedRotation = DesiredRotation;

	if (TargetCamera)
	{
		UE_LOG(
			LogVoraxiaCamera,
			Log,
			TEXT("Voraxia camera component found target camera '%s' on owner '%s'."),
			*TargetCamera->GetName(),
			*GetNameSafe(GetOwner())
		);

		ApplyCameraTransform();
	}
	else
	{
		UE_LOG(
			LogVoraxiaCamera,
			Warning,
			TEXT("Voraxia camera component has no target camera on owner '%s'."),
			*GetNameSafe(GetOwner())
		);
	}
}

void UVoraxiaCameraComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!TargetCamera)
	{
		return;
	}

	UpdateRuntimeState(DeltaTime);
	UpdateInputRotation(DeltaTime);
	ApplyCameraTransform();
}

void UVoraxiaCameraComponent::SetTargetCamera(UCameraComponent* InTargetCamera)
{
	TargetCamera = InTargetCamera;

	UE_LOG(
		LogVoraxiaCamera,
		Log,
		TEXT("Voraxia camera target changed to '%s'."),
		*GetNameSafe(TargetCamera)
	);
}

UCameraComponent* UVoraxiaCameraComponent::GetTargetCamera() const
{
	return TargetCamera;
}

void UVoraxiaCameraComponent::AddYawInput(const float Value)
{
	PendingYawInput += Value;
}

void UVoraxiaCameraComponent::AddPitchInput(const float Value)
{
	PendingPitchInput += bInvertPitchInput ? -Value : Value;
}

void UVoraxiaCameraComponent::SetFraming(
	const float NewCameraDistance,
	const float NewPivotHeight,
	const FVector NewCameraOffset,
	const FVector NewPivotOffset,
	const float NewFOVOffset,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimeCameraDistance.Set(NewCameraDistance, BlendTime, BlendCurve);
	RuntimePivotHeight.Set(NewPivotHeight, BlendTime, BlendCurve);
	RuntimeCameraOffset.Set(NewCameraOffset, BlendTime, BlendCurve);
	RuntimePivotOffset.Set(NewPivotOffset, BlendTime, BlendCurve);
	RuntimeFOVOffset.Set(NewFOVOffset, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::ResetFraming(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimeCameraDistance.Set(CameraDistance, BlendTime, BlendCurve);
	RuntimePivotHeight.Set(PivotHeight, BlendTime, BlendCurve);
	RuntimeCameraOffset.Set(FVector::ZeroVector, BlendTime, BlendCurve);
	RuntimePivotOffset.Set(FVector::ZeroVector, BlendTime, BlendCurve);
	RuntimeFOVOffset.Set(0.0f, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::SetCameraDistance(
	const float NewCameraDistance,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimeCameraDistance.Set(NewCameraDistance, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::ResetCameraDistance(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimeCameraDistance.Set(CameraDistance, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::SetPivotHeight(
	const float NewPivotHeight,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimePivotHeight.Set(NewPivotHeight, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::ResetPivotHeight(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimePivotHeight.Set(PivotHeight, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::SetCameraOffset(
	const FVector NewRuntimeCameraOffset,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimeCameraOffset.Set(NewRuntimeCameraOffset, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::ResetCameraOffset(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimeCameraOffset.Set(FVector::ZeroVector, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::SetPivotOffset(
	const FVector NewRuntimePivotOffset,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimePivotOffset.Set(NewRuntimePivotOffset, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::ResetPivotOffset(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimePivotOffset.Set(FVector::ZeroVector, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::SetFOVOffset(
	const float NewRuntimeFOVOffset,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimeFOVOffset.Set(NewRuntimeFOVOffset, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::ResetFOVOffset(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	RuntimeFOVOffset.Set(0.0f, BlendTime, BlendCurve);
}

void UVoraxiaCameraComponent::SwapCameraShoulder(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	const FVector CurrentEffectiveCameraOffset = GetCurrentCameraOffset();

	FVector NewRuntimeCameraOffset = RuntimeCameraOffset.CurrentValue;
	NewRuntimeCameraOffset.Y = -CurrentEffectiveCameraOffset.Y - AdditionalCameraOffset.Y;

	RuntimeCameraOffset.Set(NewRuntimeCameraOffset, BlendTime, BlendCurve);
}

float UVoraxiaCameraComponent::GetCurrentCameraDistance() const
{
	return RuntimeCameraDistance.CurrentValue;
}

float UVoraxiaCameraComponent::GetCurrentPivotHeight() const
{
	return RuntimePivotHeight.CurrentValue;
}

FVector UVoraxiaCameraComponent::GetCurrentCameraOffset() const
{
	return AdditionalCameraOffset + RuntimeCameraOffset.CurrentValue;
}

FVector UVoraxiaCameraComponent::GetCurrentPivotOffset() const
{
	return AdditionalPivotOffset + RuntimePivotOffset.CurrentValue;
}

float UVoraxiaCameraComponent::GetCurrentFOV() const
{
	return CalculateFinalFOV();
}

void UVoraxiaCameraComponent::InitializeRuntimeState()
{
	RuntimeCameraDistance.Initialize(CameraDistance);
	RuntimePivotHeight.Initialize(PivotHeight);
	RuntimeFOVOffset.Initialize(0.0f);

	RuntimeCameraOffset.Initialize(FVector::ZeroVector);
	RuntimePivotOffset.Initialize(FVector::ZeroVector);
}

void UVoraxiaCameraComponent::UpdateRuntimeState(const float DeltaTime)
{
	RuntimeCameraDistance.Tick(DeltaTime);
	RuntimePivotHeight.Tick(DeltaTime);
	RuntimeFOVOffset.Tick(DeltaTime);

	RuntimeCameraOffset.Tick(DeltaTime);
	RuntimePivotOffset.Tick(DeltaTime);
}

void UVoraxiaCameraComponent::UpdateInputRotation(const float DeltaTime)
{
	DesiredRotation.Yaw += PendingYawInput * YawInputSpeed * DeltaTime;
	DesiredRotation.Pitch += PendingPitchInput * PitchInputSpeed * DeltaTime;
	DesiredRotation.Pitch = FMath::Clamp(DesiredRotation.Pitch, MinPitch, MaxPitch);
	DesiredRotation.Roll = 0.0f;

	if (bEnableRotationLag && RotationLagSpeed > KINDA_SMALL_NUMBER)
	{
		SmoothedRotation = FMath::RInterpTo(
			SmoothedRotation,
			DesiredRotation,
			DeltaTime,
			RotationLagSpeed
		);
	}
	else
	{
		SmoothedRotation = DesiredRotation;
	}

	PendingYawInput = 0.0f;
	PendingPitchInput = 0.0f;
}

FVector UVoraxiaCameraComponent::CalculatePivotLocation() const
{
	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	return Owner->GetActorLocation()
		+ FVector(0.0f, 0.0f, RuntimePivotHeight.CurrentValue)
		+ GetCurrentPivotOffset();
}

FTransform UVoraxiaCameraComponent::CalculateCameraTransform() const
{
	const FVector PivotLocation = CalculatePivotLocation();

	const float Distance = FMath::Max(0.0f, RuntimeCameraDistance.CurrentValue);
	const FVector EffectiveCameraOffset = GetCurrentCameraOffset();

	const FRotationMatrix RotationMatrix(SmoothedRotation);

	const FVector Forward = RotationMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = RotationMatrix.GetUnitAxis(EAxis::Y);
	const FVector Up = RotationMatrix.GetUnitAxis(EAxis::Z);

	FVector LocalFromPivot = FVector(
		-Distance + EffectiveCameraOffset.X,
		EffectiveCameraOffset.Y,
		EffectiveCameraOffset.Z
	);

	if (bClampCameraOffsetWithinDistance && Distance > KINDA_SMALL_NUMBER)
	{
		const float LocalDistance = LocalFromPivot.Size();

		if (LocalDistance > Distance)
		{
			LocalFromPivot = LocalFromPivot.GetSafeNormal() * Distance;
		}
	}

	const FVector CameraLocation =
		PivotLocation
		+ Forward * LocalFromPivot.X
		+ Right * LocalFromPivot.Y
		+ Up * LocalFromPivot.Z;

	return FTransform(SmoothedRotation, CameraLocation);
}

void UVoraxiaCameraComponent::ApplyCameraTransform()
{
	if (!TargetCamera)
	{
		return;
	}

	const FTransform CameraTransform = CalculateCameraTransform();

	TargetCamera->SetWorldLocationAndRotation(
		CameraTransform.GetLocation(),
		CameraTransform.GetRotation().Rotator(),
		false,
		nullptr,
		ETeleportType::None
	);

	TargetCamera->SetFieldOfView(CalculateFinalFOV());
}

float UVoraxiaCameraComponent::CalculateFinalFOV() const
{
	return FMath::Clamp(
		BaseFOV + RuntimeFOVOffset.CurrentValue,
		MinFOV,
		MaxFOV
	);
}