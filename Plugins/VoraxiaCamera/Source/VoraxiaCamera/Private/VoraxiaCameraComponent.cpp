#include "VoraxiaCameraComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SVoraxiaCameraDebugPanel.h"
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
		ApplyCameraTransform(0.0f);
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
	
	if (bShowSlateDebugPanel)
	{
		CreateSlateDebugPanel();
	}
}

void UVoraxiaCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroySlateDebugPanel();

	Super::EndPlay(EndPlayReason);
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
	ApplyCameraTransform(DeltaTime);
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

bool UVoraxiaCameraComponent::IsCameraCollisionBlocked() const
{
	return bWasCameraCollisionBlocked;
}

float UVoraxiaCameraComponent::GetDesiredDistanceFromPivot() const
{
	return LastDesiredDistanceFromPivot;
}

float UVoraxiaCameraComponent::GetEffectiveDistanceFromPivot() const
{
	return LastEffectiveDistanceFromPivot;
}

float UVoraxiaCameraComponent::GetCurrentCollisionDistanceFromPivot() const
{
	return FMath::Max(0.0f, CurrentCollisionDistanceFromPivot);
}

FVector UVoraxiaCameraComponent::GetLastPivotLocation() const
{
	return LastPivotLocation;
}

FVector UVoraxiaCameraComponent::GetLastDesiredCameraLocation() const
{
	return LastDesiredCameraLocation;
}

FVector UVoraxiaCameraComponent::GetLastFinalCameraLocation() const
{
	return LastFinalCameraLocation;
}

FRotator UVoraxiaCameraComponent::GetDesiredCameraRotation() const
{
	return DesiredRotation;
}

FRotator UVoraxiaCameraComponent::GetSmoothedCameraRotation() const
{
	return SmoothedRotation;
}

FString UVoraxiaCameraComponent::GetCameraDebugSummary() const
{
	return FString::Printf(
		TEXT("VoraxiaCamera | DesiredDist: %.1f | EffectiveDist: %.1f | Collision: %s | DesiredRot P/Y: %.1f / %.1f | SmoothedRot P/Y: %.1f / %.1f | FOV: %.1f"),
		LastDesiredDistanceFromPivot,
		LastEffectiveDistanceFromPivot,
		bWasCameraCollisionBlocked ? TEXT("Blocked") : TEXT("Clear"),
		DesiredRotation.Pitch,
		DesiredRotation.Yaw,
		SmoothedRotation.Pitch,
		SmoothedRotation.Yaw,
		CalculateFinalFOV()
	);
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
	const bool bHasYawInput = FMath::Abs(PendingYawInput) > LookInputDeadZone;
	const bool bHasPitchInput = FMath::Abs(PendingPitchInput) > LookInputDeadZone;

	if (bHasYawInput)
	{
		TimeSinceLastYawInput = 0.0f;
	}
	else
	{
		TimeSinceLastYawInput += DeltaTime;
	}

	if (bHasPitchInput)
	{
		TimeSinceLastPitchInput = 0.0f;
	}
	else
	{
		TimeSinceLastPitchInput += DeltaTime;
	}

	if (bHasYawInput)
	{
		DesiredRotation.Yaw += PendingYawInput * YawInputSpeed * DeltaTime;
	}

	if (bHasPitchInput)
	{
		const float PitchInputDelta = PendingPitchInput * PitchInputSpeed * DeltaTime;
		ApplyPitchInputWithConstraints(PitchInputDelta);
	}

	UpdatePitchFollow(DeltaTime);
	UpdateYawFollow(DeltaTime);

	DesiredRotation.Roll = 0.0f;

	if (bEnablePitchConstraints)
	{
		DesiredRotation.Pitch = FMath::Clamp(DesiredRotation.Pitch, MinPitch, MaxPitch);
	}

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

void UVoraxiaCameraComponent::ApplyPitchInputWithConstraints(const float PitchInputDelta)
{
	if (!bEnablePitchConstraints || PitchConstraintTolerance <= KINDA_SMALL_NUMBER)
	{
		DesiredRotation.Pitch = FMath::Clamp(
			DesiredRotation.Pitch + PitchInputDelta,
			MinPitch,
			MaxPitch
		);

		return;
	}

	float ConstraintScale = 1.0f;

	if (PitchInputDelta < 0.0f)
	{
		const float DistanceToMin = FMath::Max(0.0f, DesiredRotation.Pitch - MinPitch);

		if (DistanceToMin < PitchConstraintTolerance)
		{
			ConstraintScale = FMath::Clamp(DistanceToMin / PitchConstraintTolerance, 0.0f, 1.0f);
		}
	}
	else if (PitchInputDelta > 0.0f)
	{
		const float DistanceToMax = FMath::Max(0.0f, MaxPitch - DesiredRotation.Pitch);

		if (DistanceToMax < PitchConstraintTolerance)
		{
			ConstraintScale = FMath::Clamp(DistanceToMax / PitchConstraintTolerance, 0.0f, 1.0f);
		}
	}

	// Smooth the soft-zone response so it does not feel like a sudden mud wall.
	ConstraintScale = ConstraintScale * ConstraintScale * (3.0f - 2.0f * ConstraintScale);

	DesiredRotation.Pitch = FMath::Clamp(
		DesiredRotation.Pitch + PitchInputDelta * ConstraintScale,
		MinPitch,
		MaxPitch
	);
}

bool UVoraxiaCameraComponent::HasRecentYawInput() const
{
	return TimeSinceLastYawInput < YawFollowTimeThreshold;
}

bool UVoraxiaCameraComponent::HasRecentPitchInput() const
{
	return TimeSinceLastPitchInput < PitchFollowTimeThreshold;
}

float UVoraxiaCameraComponent::GetOwnerMovementSpeed2D() const
{
	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return 0.0f;
	}

	return Owner->GetVelocity().Size2D();
}

float UVoraxiaCameraComponent::GetOwnerMovementYaw() const
{
	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return DesiredRotation.Yaw;
	}

	const FVector Velocity = Owner->GetVelocity();
	const FVector FlatVelocity(Velocity.X, Velocity.Y, 0.0f);

	if (FlatVelocity.IsNearlyZero())
	{
		return DesiredRotation.Yaw;
	}

	return FlatVelocity.Rotation().Yaw;
}

void UVoraxiaCameraComponent::UpdatePitchFollow(const float DeltaTime)
{
	if (!bEnablePitchMovementFollow)
	{
		return;
	}

	if (HasRecentPitchInput())
	{
		return;
	}

	if (GetOwnerMovementSpeed2D() < PitchFollowMinSpeedThreshold)
	{
		return;
	}

	const float PitchDelta = FMath::Abs(
		FMath::FindDeltaAngleDegrees(DesiredRotation.Pitch, RestingCameraPitch)
	);

	if (PitchDelta < PitchFollowAngleThreshold)
	{
		return;
	}

	const float ClampedRestingPitch = FMath::Clamp(RestingCameraPitch, MinPitch, MaxPitch);

	DesiredRotation.Pitch = FMath::FInterpTo(
		DesiredRotation.Pitch,
		ClampedRestingPitch,
		DeltaTime,
		PitchFollowSpeed
	);
}

void UVoraxiaCameraComponent::UpdateYawFollow(const float DeltaTime)
{
	if (!bEnableYawMovementFollow)
	{
		return;
	}

	if (HasRecentYawInput())
	{
		return;
	}

	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	const float Speed2D = GetOwnerMovementSpeed2D();

	if (Speed2D < YawFollowMinSpeedThreshold)
	{
		return;
	}

	if (bYawFollowOnlyForwardMovement && TargetCamera)
	{
		const FVector CameraForward = TargetCamera->GetForwardVector().GetSafeNormal2D();
		const FVector MovementDirection = Owner->GetVelocity().GetSafeNormal2D();

		const float ForwardDot = FVector::DotProduct(CameraForward, MovementDirection);

		// This prevents S/backpedal and pure strafe from dragging the camera around.
		if (ForwardDot < 0.35f)
		{
			return;
		}
	}

	const float TargetYaw = GetOwnerMovementYaw();

	const float YawDelta = FMath::Abs(
		FMath::FindDeltaAngleDegrees(DesiredRotation.Yaw, TargetYaw)
	);

	if (YawDelta < YawFollowAngleThreshold)
	{
		return;
	}

	DesiredRotation.Yaw = FMath::FixedTurn(
		DesiredRotation.Yaw,
		TargetYaw,
		YawFollowSpeed * 90.0f * DeltaTime
	);
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

FTransform UVoraxiaCameraComponent::CalculateCameraTransform(const float DeltaTime)
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

	const FVector DesiredCameraLocation =
		PivotLocation
		+ Forward * LocalFromPivot.X
		+ Right * LocalFromPivot.Y
		+ Up * LocalFromPivot.Z;

	const FVector FinalCameraLocation = ResolveCameraCollision(
		PivotLocation,
		DesiredCameraLocation,
		DeltaTime
	);

	LastPivotLocation = PivotLocation;
	LastDesiredCameraLocation = DesiredCameraLocation;
	LastFinalCameraLocation = FinalCameraLocation;

	LastDesiredDistanceFromPivot = FVector::Dist(PivotLocation, DesiredCameraLocation);
	LastEffectiveDistanceFromPivot = FVector::Dist(PivotLocation, FinalCameraLocation);

	if (bDrawCameraStateDebug)
	{
		DrawCameraStateDebug();
	}

	return FTransform(SmoothedRotation, FinalCameraLocation);
}

bool UVoraxiaCameraComponent::SweepCameraCollisionProbe(
	const FVector& PivotLocation,
	const FVector& DirectionFromPivot,
	const float DesiredDistanceFromPivot,
	const float ProbeRadius,
	float& OutSafeDistanceFromPivot,
	FHitResult& OutHit
) const
{
	OutSafeDistanceFromPivot = DesiredDistanceFromPivot;
	OutHit = FHitResult();

	if (!GetWorld() || DesiredDistanceFromPivot <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector SafeDirection = DirectionFromPivot.GetSafeNormal();

	if (SafeDirection.IsNearlyZero())
	{
		return false;
	}

	const float ClampedProbeStartOffset = FMath::Clamp(
		CollisionProbeStartOffset,
		0.0f,
		DesiredDistanceFromPivot
	);

	const FVector TraceStart = PivotLocation + SafeDirection * ClampedProbeStartOffset;
	const FVector TraceEnd = PivotLocation + SafeDirection * DesiredDistanceFromPivot;

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	if (const AActor* Owner = GetOwner())
	{
		QueryParams.AddIgnoredActor(Owner);
	}

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		CameraCollisionChannel,
		FCollisionShape::MakeSphere(FMath::Max(1.0f, ProbeRadius)),
		QueryParams
	);

	if (!bHit)
	{
		return false;
	}

	const float HitDistanceFromPivot = FVector::Dist(PivotLocation, OutHit.Location);

	OutSafeDistanceFromPivot = FMath::Clamp(
		HitDistanceFromPivot - CollisionSafetyPadding,
		FMath::Min(MinDistanceFromPivot, DesiredDistanceFromPivot),
		DesiredDistanceFromPivot
	);

	return true;
}

FVector UVoraxiaCameraComponent::ResolveCameraCollision(
	const FVector& PivotLocation,
	const FVector& DesiredCameraLocation,
	const float DeltaTime
)
{
	const FVector DesiredVector = DesiredCameraLocation - PivotLocation;
	const float DesiredDistanceFromPivot = DesiredVector.Size();

	if (DesiredDistanceFromPivot <= KINDA_SMALL_NUMBER)
	{
		CurrentCollisionDistanceFromPivot = 0.0f;
		bWasCameraCollisionBlocked = false;
		return DesiredCameraLocation;
	}

	if (!bEnableCameraCollision || !GetWorld())
	{
		CurrentCollisionDistanceFromPivot = DesiredDistanceFromPivot;
		bWasCameraCollisionBlocked = false;
		return DesiredCameraLocation;
	}

	const FVector DirectionFromPivot = DesiredVector / DesiredDistanceFromPivot;

	float SafeDistanceFromPivot = DesiredDistanceFromPivot;

	FHitResult MainHit;
	float MainSafeDistance = DesiredDistanceFromPivot;

	const bool bMainHit = SweepCameraCollisionProbe(
		PivotLocation,
		DirectionFromPivot,
		DesiredDistanceFromPivot,
		CameraCollisionRadius,
		MainSafeDistance,
		MainHit
	);

	if (bMainHit)
	{
		SafeDistanceFromPivot = FMath::Min(SafeDistanceFromPivot, MainSafeDistance);
	}

	bool bAnyFeelerHit = false;
	FHitResult BestFeelerHit;
	float BestFeelerSafeDistance = DesiredDistanceFromPivot;

	if (bEnablePredictiveAvoidance && FeelerPairs > 0)
	{
		const int32 SafeFeelerPairs = FMath::Clamp(FeelerPairs, 0, 8);
		const float SafeFeelerYawOffset = FMath::Max(0.0f, FeelerYawOffset);
		const float SafeFeelerRadius = FMath::Max(1.0f, FeelerProbeRadius);

		for (int32 PairIndex = 1; PairIndex <= SafeFeelerPairs; ++PairIndex)
		{
			const float PairYawOffset = SafeFeelerYawOffset * static_cast<float>(PairIndex);

			for (const float DirectionSign : { -1.0f, 1.0f })
			{
				const float FeelerYaw = PairYawOffset * DirectionSign;

				const FVector FeelerDirection =
					FRotator(0.0f, FeelerYaw, 0.0f).RotateVector(DirectionFromPivot).GetSafeNormal();

				FHitResult FeelerHit;
				float FeelerSafeDistance = DesiredDistanceFromPivot;

				const bool bFeelerHit = SweepCameraCollisionProbe(
					PivotLocation,
					FeelerDirection,
					DesiredDistanceFromPivot,
					SafeFeelerRadius,
					FeelerSafeDistance,
					FeelerHit
				);

				if (bDrawCameraCollisionDebug)
				{
					const FVector DebugStart =
						PivotLocation
						+ FeelerDirection
						* FMath::Clamp(CollisionProbeStartOffset, 0.0f, DesiredDistanceFromPivot);

					const FVector DebugEnd =
						PivotLocation
						+ FeelerDirection
						* DesiredDistanceFromPivot;

					DrawDebugLine(
						GetWorld(),
						DebugStart,
						DebugEnd,
						bFeelerHit ? FColor::Yellow : FColor::Cyan,
						false,
						0.0f,
						0,
						0.5f
					);

					if (bFeelerHit)
					{
						DrawDebugSphere(
							GetWorld(),
							FeelerHit.Location,
							SafeFeelerRadius,
							8,
							FColor::Yellow,
							false,
							0.0f
						);
					}
				}

				if (bFeelerHit && FeelerSafeDistance < BestFeelerSafeDistance)
				{
					bAnyFeelerHit = true;
					BestFeelerHit = FeelerHit;
					BestFeelerSafeDistance = FeelerSafeDistance;
				}
			}
		}
	}

	if (bAnyFeelerHit)
	{
		SafeDistanceFromPivot = FMath::Min(SafeDistanceFromPivot, BestFeelerSafeDistance);
	}

	if (CurrentCollisionDistanceFromPivot < 0.0f || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		CurrentCollisionDistanceFromPivot = SafeDistanceFromPivot;
	}
	else
	{
		const bool bCompressing = SafeDistanceFromPivot < CurrentCollisionDistanceFromPivot;

		const float InterpSpeed = bCompressing
			? CollisionCompressionSpeed
			: CollisionRecoverySpeed;

		CurrentCollisionDistanceFromPivot = FMath::FInterpTo(
			CurrentCollisionDistanceFromPivot,
			SafeDistanceFromPivot,
			DeltaTime,
			InterpSpeed
		);
	}

	CurrentCollisionDistanceFromPivot = FMath::Clamp(
		CurrentCollisionDistanceFromPivot,
		FMath::Min(MinDistanceFromPivot, DesiredDistanceFromPivot),
		DesiredDistanceFromPivot
	);

	bWasCameraCollisionBlocked = bMainHit || bAnyFeelerHit;

	const FVector FinalCameraLocation =
		PivotLocation + DirectionFromPivot * CurrentCollisionDistanceFromPivot;

	if (bDrawCameraCollisionDebug)
	{
		const float ClampedProbeStartOffset = FMath::Clamp(
			CollisionProbeStartOffset,
			0.0f,
			DesiredDistanceFromPivot
		);

		const FVector TraceStart = PivotLocation + DirectionFromPivot * ClampedProbeStartOffset;
		const FVector TraceEnd = DesiredCameraLocation;

		const FColor MainLineColor = bMainHit ? FColor::Red : FColor::Green;

		DrawDebugLine(
			GetWorld(),
			TraceStart,
			TraceEnd,
			MainLineColor,
			false,
			0.0f,
			0,
			1.5f
		);

		DrawDebugSphere(
			GetWorld(),
			FinalCameraLocation,
			CameraCollisionRadius,
			12,
			MainLineColor,
			false,
			0.0f
		);

		if (bMainHit)
		{
			DrawDebugSphere(
				GetWorld(),
				MainHit.Location,
				CameraCollisionRadius,
				12,
				FColor::Orange,
				false,
				0.0f
			);
		}

		if (bAnyFeelerHit)
		{
			DrawDebugSphere(
				GetWorld(),
				BestFeelerHit.Location,
				FeelerProbeRadius,
				12,
				FColor::Purple,
				false,
				0.0f
			);
		}
	}

	return FinalCameraLocation;
}

void UVoraxiaCameraComponent::ApplyCameraTransform(const float DeltaTime)
{
	if (!TargetCamera)
	{
		return;
	}

	const FTransform CameraTransform = CalculateCameraTransform(DeltaTime);

	TargetCamera->SetWorldLocationAndRotation(
		CameraTransform.GetLocation(),
		CameraTransform.GetRotation().Rotator(),
		false,
		nullptr,
		ETeleportType::None
	);

	TargetCamera->SetFieldOfView(CalculateFinalFOV());
}

void UVoraxiaCameraComponent::DrawCameraStateDebug() const
{
	if (!GetWorld())
	{
		return;
	}

	DrawDebugSphere(
		GetWorld(),
		LastPivotLocation,
		10.0f,
		12,
		FColor::Blue,
		false,
		0.0f
	);

	DrawDebugSphere(
		GetWorld(),
		LastDesiredCameraLocation,
		10.0f,
		12,
		FColor::Cyan,
		false,
		0.0f
	);

	DrawDebugSphere(
		GetWorld(),
		LastFinalCameraLocation,
		12.0f,
		12,
		bWasCameraCollisionBlocked ? FColor::Red : FColor::Green,
		false,
		0.0f
	);

	DrawDebugLine(
		GetWorld(),
		LastPivotLocation,
		LastDesiredCameraLocation,
		FColor::Cyan,
		false,
		0.0f,
		0,
		0.75f
	);

	DrawDebugLine(
		GetWorld(),
		LastPivotLocation,
		LastFinalCameraLocation,
		bWasCameraCollisionBlocked ? FColor::Red : FColor::Green,
		false,
		0.0f,
		0,
		1.5f
	);

	DrawDebugCoordinateSystem(
		GetWorld(),
		LastPivotLocation,
		SmoothedRotation,
		40.0f,
		false,
		0.0f,
		0,
		1.0f
	);
}

void UVoraxiaCameraComponent::SetSlateDebugPanelVisible(const bool bVisible)
{
	bShowSlateDebugPanel = bVisible;

	if (bShowSlateDebugPanel)
	{
		CreateSlateDebugPanel();
	}
	else
	{
		DestroySlateDebugPanel();
	}
}

void UVoraxiaCameraComponent::ToggleSlateDebugPanel()
{
	SetSlateDebugPanelVisible(!IsSlateDebugPanelVisible());
}

bool UVoraxiaCameraComponent::IsSlateDebugPanelVisible() const
{
	return SlateDebugPanelWidget.IsValid();
}

void UVoraxiaCameraComponent::CreateSlateDebugPanel()
{
	if (SlateDebugPanelWidget.IsValid())
	{
		return;
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedRef<SWidget> PanelWidget =
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		[
			SNew(SVoraxiaCameraDebugPanel)
			.CameraComponent(TWeakObjectPtr<UVoraxiaCameraComponent>(this))
			.PanelWidth(SlateDebugPanelWidth)
		];

	SlateDebugPanelWidget = PanelWidget;

	GEngine->GameViewport->AddViewportWidgetContent(
		PanelWidget,
		SlateDebugPanelZOrder
	);
}

void UVoraxiaCameraComponent::DestroySlateDebugPanel()
{
	if (SlateDebugPanelWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(
			SlateDebugPanelWidget.ToSharedRef()
		);
	}

	SlateDebugPanelWidget.Reset();
}

float UVoraxiaCameraComponent::CalculateFinalFOV() const
{
	return FMath::Clamp(
		BaseFOV + RuntimeFOVOffset.CurrentValue,
		MinFOV,
		MaxFOV
	);
}