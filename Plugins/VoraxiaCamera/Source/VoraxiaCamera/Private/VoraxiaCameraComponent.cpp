// Copyright 2026 Coding Custard Studios.

#include "VoraxiaCameraComponent.h"

#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SBoxPanel.h"
#include "Components/SceneComponent.h"
#include "Widgets/SVoraxiaCameraDebugPanel.h"
#include "Interfaces/VoraxiaFocusableInterface.h"
#include "Interfaces/VoraxiaScannableInterface.h"
#include "VoraxiaCameraLog.h"
#include "VoraxiaCameraOcclusionDitherComponent.h"
#include "VoraxiaCameraSettingsAsset.h"

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

	/*
	 * On network clients, a Pawn can begin play before local possession is
	 * established. Do not permanently disable this component here: TickComponent
	 * will initialise it the first frame that this machine owns the Pawn.
	 */
	if (IsLocalPresentationOwner())
	{
		InitializeLocalPresentation();
	}
}

void UVoraxiaCameraComponent::InitializeLocalPresentation()
{
	if (bLocalPresentationInitialized || !IsLocalPresentationOwner())
	{
		return;
	}

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

	const FRotator OwnerRotation =
		GetOwner() ? GetOwner()->GetActorRotation() : FRotator::ZeroRotator;

	DesiredRotation = FRotator(
		FMath::Clamp(InitialPitch, MinPitch, MaxPitch),
		OwnerRotation.Yaw,
		0.0f
	);

	SmoothedRotation = DesiredRotation;
	UnfocusedDesiredRotation = DesiredRotation;
	ResetYawConstraintState();

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

	bLocalPresentationInitialized = true;

	if (bShowSlateDebugPanel)
	{
		CreateSlateDebugPanel();
	}
}

void UVoraxiaCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllCameraShakes(true);
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

	if (!IsLocalPresentationOwner())
	{
		return;
	}

	if (!bLocalPresentationInitialized)
	{
		InitializeLocalPresentation();
	}

	if (!bLocalPresentationInitialized || !TargetCamera)
	{
		return;
	}

	UpdateRuntimeState(DeltaTime);
	UpdateYawConstraintState(DeltaTime);
	UpdateInputRotation(DeltaTime);
	UpdateDynamicModifiers(DeltaTime);
	UpdateMovementAnticipation(DeltaTime);
	UpdateCameraShakeSystem(DeltaTime);
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


FVoraxiaCameraShakeHandle UVoraxiaCameraComponent::PlayCameraShake(
	TSubclassOf<UCameraShakeBase> ShakeClass,
	const float Scale,
	const ECameraShakePlaySpace PlaySpace,
	const FRotator UserPlaySpaceRotation
)
{
	FVoraxiaCameraShakeHandle InvalidHandle;

	if (!IsLocalPresentationOwner())
	{
		return InvalidHandle;
	}

	if (!ShakeClass)
	{
		UE_LOG(LogVoraxiaCamera, Warning, TEXT("Voraxia camera shake request ignored because no shake class was supplied."));
		return InvalidHandle;
	}

	APlayerCameraManager* CameraManager = ResolvePlayerCameraManager();

	if (!CameraManager)
	{
		UE_LOG(
			LogVoraxiaCamera,
			Warning,
			TEXT("Voraxia camera shake request for '%s' ignored because owner '%s' has no local PlayerCameraManager."),
			*GetNameSafe(ShakeClass.Get()),
			*GetNameSafe(GetOwner())
		);
		return InvalidHandle;
	}

	UCameraShakeBase* Instance = CameraManager->StartCameraShake(
		ShakeClass,
		FMath::Max(0.0f, Scale),
		PlaySpace,
		UserPlaySpaceRotation
	);

	if (!Instance)
	{
		UE_LOG(LogVoraxiaCamera, Warning, TEXT("Voraxia camera shake '%s' failed to start."), *GetNameSafe(ShakeClass.Get()));
		return InvalidHandle;
	}

	for (const TPair<int32, FActiveCameraShake>& Pair : ActiveCameraShakes)
	{
		if (Pair.Value.Instance.Get() == Instance)
		{
			// A single-instance shake class may reuse an existing instance. Preserve its original handle.
			return Pair.Value.Handle;
		}
	}

	FActiveCameraShake& ActiveShake = ActiveCameraShakes.Add(NextCameraShakeHandleId);
	ActiveShake.Handle.Id = NextCameraShakeHandleId++;
	ActiveShake.Handle.ShakeName = ShakeClass.Get()->GetFName();
	ActiveShake.Instance = Instance;
	ActiveShake.ShakeClass = ShakeClass;

	OnCameraShakeStarted.Broadcast(ActiveShake.Handle, FMath::Max(0.0f, Scale));

	UE_LOG(
		LogVoraxiaCamera,
		Verbose,
		TEXT("Voraxia camera shake started. Handle=%d Shake='%s' Scale=%.2f."),
		ActiveShake.Handle.Id,
		*ActiveShake.Handle.ShakeName.ToString(),
		Scale
	);

	return ActiveShake.Handle;
}

bool UVoraxiaCameraComponent::StopCameraShake(
	const FVoraxiaCameraShakeHandle ShakeHandle,
	const bool bImmediately
)
{
	if (!ShakeHandle.IsValid() || !ActiveCameraShakes.Contains(ShakeHandle.Id))
	{
		return false;
	}

	RequestStopCameraShake(ShakeHandle.Id, bImmediately);
	return true;
}

void UVoraxiaCameraComponent::StopAllCameraShakes(const bool bImmediately)
{
	TArray<int32> HandleIds;
	ActiveCameraShakes.GetKeys(HandleIds);

	for (const int32 HandleId : HandleIds)
	{
		RequestStopCameraShake(HandleId, bImmediately);
	}
}

bool UVoraxiaCameraComponent::IsCameraShakePlaying(const FVoraxiaCameraShakeHandle ShakeHandle) const
{
	const FActiveCameraShake* ActiveShake = ActiveCameraShakes.Find(ShakeHandle.Id);

	if (!ActiveShake)
	{
		return false;
	}

	UCameraShakeBase* Instance = ActiveShake->Instance.Get();
	return Instance && Instance->IsActive() && !Instance->IsFinished();
}

int32 UVoraxiaCameraComponent::GetActiveCameraShakeCount() const
{
	return ActiveCameraShakes.Num();
}

bool UVoraxiaCameraComponent::IsLocalPresentationOwner() const
{
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	return OwningPawn && OwningPawn->IsLocallyControlled();
}

APlayerCameraManager* UVoraxiaCameraComponent::ResolvePlayerCameraManager() const
{
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwningPawn
		? Cast<APlayerController>(OwningPawn->GetController())
		: nullptr;

	return PlayerController ? PlayerController->PlayerCameraManager : nullptr;
}

void UVoraxiaCameraComponent::UpdateCameraShakeSystem(const float DeltaTime)
{
	UpdateActiveCameraShakes();
	UpdateAutomaticCameraShakes(DeltaTime);
}

void UVoraxiaCameraComponent::UpdateActiveCameraShakes()
{
	for (auto It = ActiveCameraShakes.CreateIterator(); It; ++It)
	{
		const FActiveCameraShake& ActiveShake = It.Value();
		UCameraShakeBase* Instance = ActiveShake.Instance.Get();

		if (Instance && Instance->IsActive() && !Instance->IsFinished())
		{
			continue;
		}

		NotifyCameraShakeStopped(ActiveShake, ActiveShake.bStopRequested);

		if (IdleCameraShakeHandle.Id == ActiveShake.Handle.Id)
		{
			IdleCameraShakeHandle = FVoraxiaCameraShakeHandle();
		}

		if (MovementCameraShakeHandle.Id == ActiveShake.Handle.Id)
		{
			MovementCameraShakeHandle = FVoraxiaCameraShakeHandle();
		}

		It.RemoveCurrent();
	}
}

void UVoraxiaCameraComponent::UpdateAutomaticCameraShakes(const float DeltaTime)
{
	const float OwnerSpeed = GetOwnerMovementSpeed2D();
	const bool bShouldIdleShake = bEnableIdleCameraShake
		&& IdleCameraShakeClass
		&& OwnerSpeed <= FMath::Max(0.0f, IdleCameraShakeMaxSpeed);

	if (bShouldIdleShake)
	{
		if (!IsCameraShakePlaying(IdleCameraShakeHandle))
		{
			IdleCameraShakeHandle = PlayCameraShake(
				IdleCameraShakeClass,
				IdleCameraShakeScale,
				ECameraShakePlaySpace::CameraLocal
			);
		}
	}
	else if (IdleCameraShakeHandle.IsValid())
	{
		StopCameraShake(IdleCameraShakeHandle, false);
	}

	const float SafeMinSpeed = FMath::Max(0.0f, MovementCameraShakeMinSpeed);
	const float SafeMaxSpeed = FMath::Max(SafeMinSpeed + KINDA_SMALL_NUMBER, MovementCameraShakeMaxSpeed);
	const bool bShouldMovementShake = bEnableMovementCameraShake
		&& MovementCameraShakeClass
		&& OwnerSpeed >= SafeMinSpeed;

	const float TargetMovementScale = bShouldMovementShake
		? FMath::Lerp(
			MovementCameraShakeMinScale,
			MovementCameraShakeMaxScale,
			FMath::Clamp((OwnerSpeed - SafeMinSpeed) / (SafeMaxSpeed - SafeMinSpeed), 0.0f, 1.0f)
		)
		: 0.0f;

	CurrentMovementCameraShakeScale = FMath::FInterpTo(
		CurrentMovementCameraShakeScale,
		TargetMovementScale,
		FMath::Max(0.0f, DeltaTime),
		FMath::Max(0.0f, MovementCameraShakeScaleInterpSpeed)
	);

	if (bShouldMovementShake && !IsCameraShakePlaying(MovementCameraShakeHandle))
	{
		MovementCameraShakeHandle = PlayCameraShake(
			MovementCameraShakeClass,
			CurrentMovementCameraShakeScale,
			ECameraShakePlaySpace::CameraLocal
		);
	}

	if (FActiveCameraShake* MovementShake = ActiveCameraShakes.Find(MovementCameraShakeHandle.Id))
	{
		if (UCameraShakeBase* Instance = MovementShake->Instance.Get())
		{
			Instance->ShakeScale = CurrentMovementCameraShakeScale;
		}
	}

	if (!bShouldMovementShake && MovementCameraShakeHandle.IsValid())
	{
		StopCameraShake(MovementCameraShakeHandle, false);
	}
}

void UVoraxiaCameraComponent::RequestStopCameraShake(const int32 HandleId, const bool bImmediately)
{
	FActiveCameraShake* ActiveShake = ActiveCameraShakes.Find(HandleId);

	if (!ActiveShake || ActiveShake->bStopRequested)
	{
		return;
	}

	ActiveShake->bStopRequested = true;
	ActiveShake->bStopWasImmediate = bImmediately;

	if (UCameraShakeBase* Instance = ActiveShake->Instance.Get())
	{
		if (APlayerCameraManager* CameraManager = ResolvePlayerCameraManager())
		{
			CameraManager->StopCameraShake(Instance, bImmediately);
		}
		else
		{
			Instance->StopShake(bImmediately);
		}
	}
}

void UVoraxiaCameraComponent::NotifyCameraShakeStopped(
	const FActiveCameraShake& ActiveShake,
	const bool bInterrupted
)
{
	OnCameraShakeStopped.Broadcast(ActiveShake.Handle, bInterrupted);

	UE_LOG(
		LogVoraxiaCamera,
		Verbose,
		TEXT("Voraxia camera shake stopped. Handle=%d Shake='%s' Interrupted=%s."),
		ActiveShake.Handle.Id,
		*ActiveShake.Handle.ShakeName.ToString(),
		bInterrupted ? TEXT("true") : TEXT("false")
	);
}

void UVoraxiaCameraComponent::AddYawInput(const float Value)
{
	PendingYawInput += Value;
}

void UVoraxiaCameraComponent::AddPitchInput(const float Value)
{
	PendingPitchInput += bInvertPitchInput ? -Value : Value;
}


void UVoraxiaCameraComponent::SetYawConstraint(
	const float ReferenceYaw,
	const float MinYawDelta,
	const float MaxYawDelta,
	const float BlendTime
)
{
	const float SafeMinYawDelta = FMath::Clamp(
		FMath::Min(MinYawDelta, MaxYawDelta),
		-180.0f,
		180.0f
	);

	const float SafeMaxYawDelta = FMath::Clamp(
		FMath::Max(MinYawDelta, MaxYawDelta),
		-180.0f,
		180.0f
	);

	if (!IsYawConstraintActive())
	{
		CurrentYawConstraintReferenceYaw = FMath::UnwindDegrees(DesiredRotation.Yaw);
		CurrentYawConstraintMinDelta = -180.0f;
		CurrentYawConstraintMaxDelta = 180.0f;
		CurrentYawConstraintAlpha = 0.0f;
	}

	BeginYawConstraintBlend(
		ReferenceYaw,
		SafeMinYawDelta,
		SafeMaxYawDelta,
		1.0f,
		BlendTime
	);

	if (BlendTime <= KINDA_SMALL_NUMBER)
	{
		DesiredRotation.Yaw = ClampYawToConstraint(DesiredRotation.Yaw);
		UnfocusedDesiredRotation.Yaw = DesiredRotation.Yaw;
	}
}

void UVoraxiaCameraComponent::SetYawConstraintAroundCurrentView(
	const float MinYawDelta,
	const float MaxYawDelta,
	const float BlendTime
)
{
	SetYawConstraint(
		DesiredRotation.Yaw,
		MinYawDelta,
		MaxYawDelta,
		BlendTime
	);
}

void UVoraxiaCameraComponent::ClearYawConstraint(const float BlendTime)
{
	if (!IsYawConstraintActive())
	{
		return;
	}

	BeginYawConstraintBlend(
		CurrentYawConstraintReferenceYaw,
		-180.0f,
		180.0f,
		0.0f,
		BlendTime
	);
}

bool UVoraxiaCameraComponent::IsYawConstraintActive() const
{
	return bYawConstraintTargetActive
		|| CurrentYawConstraintAlpha > KINDA_SMALL_NUMBER
		|| YawConstraintTargetAlpha > KINDA_SMALL_NUMBER;
}

float UVoraxiaCameraComponent::GetYawConstraintReferenceYaw() const
{
	return CurrentYawConstraintReferenceYaw;
}

float UVoraxiaCameraComponent::GetYawConstraintMinDelta() const
{
	return CurrentYawConstraintMinDelta;
}

float UVoraxiaCameraComponent::GetYawConstraintMaxDelta() const
{
	return CurrentYawConstraintMaxDelta;
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

void UVoraxiaCameraComponent::SetUseRightShoulder(
	const bool bNewUseRightShoulder,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	bUseRightShoulder = bNewUseRightShoulder;

	RuntimeShoulderOffset.Set(
		GetTargetShoulderOffset(),
		BlendTime,
		BlendCurve
	);
}

void UVoraxiaCameraComponent::SetShoulderOffset(
	const float NewShoulderOffset,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	ShoulderOffset = FMath::Max(0.0f, NewShoulderOffset);

	RuntimeShoulderOffset.Set(
		GetTargetShoulderOffset(),
		BlendTime,
		BlendCurve
	);
}

void UVoraxiaCameraComponent::SwapCameraShoulder(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	SetUseRightShoulder(
		!bUseRightShoulder,
		BlendTime,
		BlendCurve
	);
}

bool UVoraxiaCameraComponent::IsUsingRightShoulder() const
{
	return bUseRightShoulder;
}

float UVoraxiaCameraComponent::GetCurrentShoulderOffset() const
{
	return RuntimeShoulderOffset.CurrentValue;
}

void UVoraxiaCameraComponent::SetFocusActor(
	AActor* NewFocusActor,
	const float BlendTime,
	const FName SocketName
)
{
	if (!bEnableFocusSystem || !NewFocusActor)
	{
		ClearFocus(BlendTime);
		return;
	}

	FocusTargetActor = NewFocusActor;
	FocusTargetComponent.Reset();
	FocusTargetSocketName = SocketName;

	bFocusUsesWorldLocation = false;
	bFocusActive = true;
	bHasCachedFocusLocation = false;

	BeginFocusBlend(
		1.0f,
		BlendTime >= 0.0f ? BlendTime : DefaultFocusBlendInTime
	);

	LogScannableFocusTarget(NewFocusActor);
}

void UVoraxiaCameraComponent::SetFocusComponent(
	USceneComponent* NewFocusComponent,
	const float BlendTime,
	const FName SocketName
)
{
	if (!bEnableFocusSystem || !NewFocusComponent)
	{
		ClearFocus(BlendTime);
		return;
	}

	FocusTargetActor.Reset();
	FocusTargetComponent = NewFocusComponent;
	FocusTargetSocketName = SocketName;

	bFocusUsesWorldLocation = false;
	bFocusActive = true;
	bHasCachedFocusLocation = false;

	BeginFocusBlend(
		1.0f,
		BlendTime >= 0.0f ? BlendTime : DefaultFocusBlendInTime
	);
}

void UVoraxiaCameraComponent::SetFocusWorldLocation(
	const FVector NewFocusWorldLocation,
	const float BlendTime
)
{
	if (!bEnableFocusSystem)
	{
		ClearFocus(BlendTime);
		return;
	}

	FocusTargetActor.Reset();
	FocusTargetComponent.Reset();
	FocusTargetSocketName = NAME_None;

	FocusWorldLocation = NewFocusWorldLocation;
	CachedFocusLocation = NewFocusWorldLocation;

	bFocusUsesWorldLocation = true;
	bFocusActive = true;
	bHasCachedFocusLocation = true;

	BeginFocusBlend(
		1.0f,
		BlendTime >= 0.0f ? BlendTime : DefaultFocusBlendInTime
	);
}


void UVoraxiaCameraComponent::ClearFocus(const float BlendTime)
{
	FVector CurrentFocusLocation;

	if (TryGetFocusLocation(CurrentFocusLocation))
	{
		CachedFocusLocation = CurrentFocusLocation;
		bHasCachedFocusLocation = true;
	}

	bFocusActive = false;
	bFocusUsesWorldLocation = false;

	FocusTargetActor.Reset();
	FocusTargetComponent.Reset();
	FocusTargetSocketName = NAME_None;

	BeginFocusBlend(
		0.0f,
		BlendTime >= 0.0f ? BlendTime : DefaultFocusBlendOutTime
	);
}

bool UVoraxiaCameraComponent::IsFocusActive() const
{
	return bFocusActive || FocusAlpha > KINDA_SMALL_NUMBER;
}

float UVoraxiaCameraComponent::GetCurrentFocusAlpha() const
{
	return FocusAlpha;
}

FVector UVoraxiaCameraComponent::GetCurrentFocusLocation() const
{
	FVector CurrentFocusLocation;

	if (TryGetFocusLocation(CurrentFocusLocation))
	{
		return CurrentFocusLocation;
	}

	return FVector::ZeroVector;
}

bool UVoraxiaCameraComponent::HasFocusTarget() const
{
	return FocusTargetActor.IsValid()
		|| FocusTargetComponent.IsValid()
		|| bFocusUsesWorldLocation
		|| bHasCachedFocusLocation;
}

AActor* UVoraxiaCameraComponent::GetCurrentFocusActor() const
{
	return FocusTargetActor.Get();
}

USceneComponent* UVoraxiaCameraComponent::GetCurrentFocusComponent() const
{
	return FocusTargetComponent.Get();
}

float UVoraxiaCameraComponent::GetCurrentPitchDistanceOffset() const
{
	return CurrentPitchDistanceOffset;
}

float UVoraxiaCameraComponent::GetCurrentPitchFOVOffset() const
{
	return CurrentPitchFOVOffset;
}

float UVoraxiaCameraComponent::GetCurrentSpeedDistanceOffset() const
{
	return CurrentSpeedDistanceOffset;
}

float UVoraxiaCameraComponent::GetCurrentSpeedFOVOffset() const
{
	return CurrentSpeedFOVOffset;
}

float UVoraxiaCameraComponent::GetCurrentDynamicDistanceOffset() const
{
	return CurrentPitchDistanceOffset + CurrentSpeedDistanceOffset;
}

float UVoraxiaCameraComponent::GetCurrentDynamicFOVOffset() const
{
	return CurrentPitchFOVOffset + CurrentSpeedFOVOffset;
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
	FVector CurrentCameraOffset = AdditionalCameraOffset + RuntimeCameraOffset.CurrentValue;
	CurrentCameraOffset.Y += RuntimeShoulderOffset.CurrentValue;

	return CurrentCameraOffset;
}

FVector UVoraxiaCameraComponent::GetCurrentPivotOffset() const
{
	return AdditionalPivotOffset + RuntimePivotOffset.CurrentValue;
}

float UVoraxiaCameraComponent::GetCurrentFOV() const
{
	return CalculateFinalFOV();
}

FVector UVoraxiaCameraComponent::GetCurrentMovementAnticipationOffset() const
{
	return CurrentMovementAnticipationOffset;
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

FString UVoraxiaCameraComponent::GetCurrentFocusTargetName() const
{
	if (AActor* Actor = FocusTargetActor.Get())
	{
		if (Actor->GetClass()->ImplementsInterface(UVoraxiaFocusableInterface::StaticClass()))
		{
			return IVoraxiaFocusableInterface::Execute_GetFocusDisplayName(Actor).ToString();
		}

		return Actor->GetName();
	}

	if (const USceneComponent* Component = FocusTargetComponent.Get())
	{
		return Component->GetName();
	}

	if (bFocusUsesWorldLocation)
	{
		return TEXT("World Location");
	}

	if (bHasCachedFocusLocation)
	{
		return TEXT("Cached Focus");
	}

	return TEXT("None");
}

bool UVoraxiaCameraComponent::IsCurrentFocusTargetScannable() const
{
	AActor* Actor = FocusTargetActor.Get();

	if (!Actor)
	{
		return false;
	}

	if (!Actor->GetClass()->ImplementsInterface(UVoraxiaScannableInterface::StaticClass()))
	{
		return false;
	}

	return IVoraxiaScannableInterface::Execute_CanBeScanned(
		Actor,
		GetOwner()
	);
}

FText UVoraxiaCameraComponent::GetCurrentFocusScanDisplayName() const
{
	AActor* Actor = FocusTargetActor.Get();

	if (!Actor || !Actor->GetClass()->ImplementsInterface(UVoraxiaScannableInterface::StaticClass()))
	{
		return FText::FromString(TEXT("None"));
	}

	return IVoraxiaScannableInterface::Execute_GetScanDisplayName(Actor);
}

FText UVoraxiaCameraComponent::GetCurrentFocusScanSummary() const
{
	AActor* Actor = FocusTargetActor.Get();

	if (!Actor || !Actor->GetClass()->ImplementsInterface(UVoraxiaScannableInterface::StaticClass()))
	{
		return FText::FromString(TEXT("-"));
	}

	return IVoraxiaScannableInterface::Execute_GetScanSummary(Actor);
}

float UVoraxiaCameraComponent::GetCurrentFocusScanTimeSeconds() const
{
	AActor* Actor = FocusTargetActor.Get();

	if (!Actor || !Actor->GetClass()->ImplementsInterface(UVoraxiaScannableInterface::StaticClass()))
	{
		return 0.0f;
	}

	return IVoraxiaScannableInterface::Execute_GetScanTimeSeconds(Actor);
}

FString UVoraxiaCameraComponent::GetCurrentFocusScanCompositionSummary() const
{
	AActor* Actor = FocusTargetActor.Get();

	if (!Actor || !Actor->GetClass()->ImplementsInterface(UVoraxiaScannableInterface::StaticClass()))
	{
		return TEXT("-");
	}

	const TArray<FVoraxiaScanCompositionEntry> Composition =
		IVoraxiaScannableInterface::Execute_GetScanComposition(Actor);

	if (Composition.Num() <= 0)
	{
		return TEXT("No composition entries.");
	}

	TArray<FString> Lines;
	Lines.Reserve(Composition.Num());

	for (const FVoraxiaScanCompositionEntry& Entry : Composition)
	{
		const FString MaterialIdString = Entry.MaterialId.ToString();
		const FString DisplayNameString = Entry.DisplayName.ToString();

		const FString DisplayName =
			DisplayNameString.IsEmpty()
				? MaterialIdString
				: DisplayNameString;

		Lines.Add(FString::Printf(
			TEXT("%s [%s]: %.1f%%"),
			*DisplayName,
			*MaterialIdString,
			Entry.Percentage
		));
	}

	return FString::Join(Lines, TEXT("\n"));
}

void UVoraxiaCameraComponent::LogScannableFocusTarget(AActor* Actor) const
{
	if (!Actor)
	{
		return;
	}

	if (!Actor->GetClass()->ImplementsInterface(UVoraxiaScannableInterface::StaticClass()))
	{
		UE_LOG(
			LogVoraxiaCamera,
			Verbose,
			TEXT("Voraxia focus target '%s' is not scannable."),
			*GetNameSafe(Actor)
		);

		return;
	}

	if (!IVoraxiaScannableInterface::Execute_CanBeScanned(Actor, GetOwner()))
	{
		UE_LOG(
			LogVoraxiaCamera,
			Log,
			TEXT("Voraxia focus target '%s' implements scan data but cannot currently be scanned."),
			*GetNameSafe(Actor)
		);

		return;
	}

	const FText ScanDisplayName = IVoraxiaScannableInterface::Execute_GetScanDisplayName(Actor);
	const FText ScanSummary = IVoraxiaScannableInterface::Execute_GetScanSummary(Actor);
	const float ScanTimeSeconds = IVoraxiaScannableInterface::Execute_GetScanTimeSeconds(Actor);

	const FString ScanDisplayNameString = ScanDisplayName.ToString();
	const FString ScanSummaryString = ScanSummary.ToString();

	UE_LOG(
		LogVoraxiaCamera,
		Log,
		TEXT("Voraxia scan target acquired: '%s' on actor '%s'. Scan time: %.2fs."),
		*ScanDisplayNameString,
		*GetNameSafe(Actor),
		ScanTimeSeconds
	);

	UE_LOG(
		LogVoraxiaCamera,
		Log,
		TEXT("Voraxia scan summary: %s"),
		*ScanSummaryString
	);

	const TArray<FVoraxiaScanCompositionEntry> Composition =
		IVoraxiaScannableInterface::Execute_GetScanComposition(Actor);

	if (Composition.Num() <= 0)
	{
		UE_LOG(
			LogVoraxiaCamera,
			Log,
			TEXT("Voraxia scan composition: no composition entries.")
		);

		return;
	}

	UE_LOG(
		LogVoraxiaCamera,
		Log,
		TEXT("Voraxia scan composition:")
	);

	for (const FVoraxiaScanCompositionEntry& Entry : Composition)
	{
		const FString MaterialIdString = Entry.MaterialId.ToString();
		const FString EntryDisplayNameString = Entry.DisplayName.ToString();

		const FString DisplayName =
			EntryDisplayNameString.IsEmpty()
				? MaterialIdString
				: EntryDisplayNameString;

		UE_LOG(
			LogVoraxiaCamera,
			Log,
			TEXT("  - %s [%s]: %.1f%%"),
			*DisplayName,
			*MaterialIdString,
			Entry.Percentage
		);
	}
}

FString UVoraxiaCameraComponent::GetCameraDebugSummary() const
{
	return FString::Printf(
		TEXT("VoraxiaCamera | DesiredDist: %.1f | EffectiveDist: %.1f | DynDist: %.1f | Collision: %s | Focus: %s %.2f '%s' | Scan: %s | YawLimit: %s [%.1f, %.1f] | DesiredRot P/Y: %.1f / %.1f | SmoothedRot P/Y: %.1f / %.1f | DynFOV: %.1f | FOV: %.1f"),
		LastDesiredDistanceFromPivot,
		LastEffectiveDistanceFromPivot,
		GetCurrentDynamicDistanceOffset(),
		bWasCameraCollisionBlocked ? TEXT("Blocked") : TEXT("Clear"),
		IsFocusActive() ? TEXT("Active") : TEXT("None"),
		FocusAlpha,
		*GetCurrentFocusTargetName(),
		IsCurrentFocusTargetScannable() ? TEXT("Yes") : TEXT("No"),
		IsYawConstraintActive() ? TEXT("Active") : TEXT("Clear"),
		GetYawConstraintMinDelta(),
		GetYawConstraintMaxDelta(),
		DesiredRotation.Pitch,
		DesiredRotation.Yaw,
		SmoothedRotation.Pitch,
		SmoothedRotation.Yaw,
		GetCurrentDynamicFOVOffset(),
		CalculateFinalFOV()
	);
}

bool UVoraxiaCameraComponent::CaptureCurrentSettingsToAsset(
	UVoraxiaCameraSettingsAsset* SettingsAsset
)
{
	if (!SettingsAsset)
	{
		UE_LOG(LogVoraxiaCamera, Warning, TEXT("Voraxia camera settings capture failed because no settings asset was supplied."));
		return false;
	}

#if WITH_EDITOR
	SettingsAsset->Modify();

	PopulatePersistentSettings(SettingsAsset->CameraSettings);
	SettingsAsset->bIncludeOcclusionDitherSettings = false;

	if (AActor* Owner = GetOwner())
	{
		if (UVoraxiaCameraOcclusionDitherComponent* DitherComponent = Owner->FindComponentByClass<UVoraxiaCameraOcclusionDitherComponent>())
		{
			DitherComponent->CaptureSettings(SettingsAsset->OcclusionDitherSettings);
			SettingsAsset->bIncludeOcclusionDitherSettings = true;
		}
	}

	SettingsAsset->MarkPackageDirty();
	AssignedSettingsAsset = SettingsAsset;
	OnCameraSettingsCaptured.Broadcast(SettingsAsset);

	UE_LOG(LogVoraxiaCamera, Log, TEXT("Voraxia camera settings captured into '%s'."), *GetNameSafe(SettingsAsset));
	return true;
#else
	UE_LOG(LogVoraxiaCamera, Warning, TEXT("Voraxia camera settings capture is editor-only because packaged builds cannot persist Data Assets."));
	return false;
#endif
}

bool UVoraxiaCameraComponent::ApplyCameraSettingsAsset(
	UVoraxiaCameraSettingsAsset* SettingsAsset
)
{
	if (!SettingsAsset)
	{
		UE_LOG(LogVoraxiaCamera, Warning, TEXT("Voraxia camera settings apply failed because no settings asset was supplied."));
		return false;
	}

	ApplyPersistentSettings(SettingsAsset->CameraSettings);

	if (SettingsAsset->bIncludeOcclusionDitherSettings)
	{
		if (AActor* Owner = GetOwner())
		{
			if (UVoraxiaCameraOcclusionDitherComponent* DitherComponent = Owner->FindComponentByClass<UVoraxiaCameraOcclusionDitherComponent>())
			{
				DitherComponent->ApplySettings(SettingsAsset->OcclusionDitherSettings);
			}
		}
	}

	AssignedSettingsAsset = SettingsAsset;
	OnCameraSettingsApplied.Broadcast(SettingsAsset);

	UE_LOG(LogVoraxiaCamera, Log, TEXT("Voraxia camera settings applied from '%s'."), *GetNameSafe(SettingsAsset));
	return true;
}

bool UVoraxiaCameraComponent::ApplyCameraSettingsAssetSmooth(
	UVoraxiaCameraSettingsAsset* SettingsAsset,
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	if (!SettingsAsset)
	{
		UE_LOG(LogVoraxiaCamera, Warning, TEXT("Voraxia smooth camera settings apply failed because no settings asset was supplied."));
		return false;
	}

	const float SafeBlendTime = FMath::Max(0.0f, BlendTime);

	if (SafeBlendTime <= KINDA_SMALL_NUMBER)
	{
		return ApplyCameraSettingsAsset(SettingsAsset);
	}

	/*
	 * Preserve the visible framing before persistent settings change.
	 * Runtime offsets are rebased and then blended to zero, so new base offsets do not snap.
	 */
	const FVector CurrentVisibleCameraOffset =
		AdditionalCameraOffset + RuntimeCameraOffset.CurrentValue;

	const FVector CurrentVisiblePivotOffset =
		AdditionalPivotOffset + RuntimePivotOffset.CurrentValue;

	const float CurrentVisibleBaseFOV =
		BaseFOV + RuntimeFOVOffset.CurrentValue;

	ApplyPersistentSettings(SettingsAsset->CameraSettings, false);

	RuntimeCameraDistance.Set(CameraDistance, SafeBlendTime, BlendCurve);
	RuntimePivotHeight.Set(PivotHeight, SafeBlendTime, BlendCurve);

	RuntimeCameraOffset.Initialize(
		CurrentVisibleCameraOffset - AdditionalCameraOffset
	);
	RuntimeCameraOffset.Set(FVector::ZeroVector, SafeBlendTime, BlendCurve);

	RuntimePivotOffset.Initialize(
		CurrentVisiblePivotOffset - AdditionalPivotOffset
	);
	RuntimePivotOffset.Set(FVector::ZeroVector, SafeBlendTime, BlendCurve);

	RuntimeFOVOffset.Initialize(CurrentVisibleBaseFOV - BaseFOV);
	RuntimeFOVOffset.Set(0.0f, SafeBlendTime, BlendCurve);

	RuntimeShoulderOffset.Set(
		GetTargetShoulderOffset(),
		SafeBlendTime,
		BlendCurve
	);

	if (bShowSlateDebugPanel)
	{
		CreateSlateDebugPanel();
	}
	else
	{
		DestroySlateDebugPanel();
	}

	if (SettingsAsset->bIncludeOcclusionDitherSettings)
	{
		if (AActor* Owner = GetOwner())
		{
			if (UVoraxiaCameraOcclusionDitherComponent* DitherComponent = Owner->FindComponentByClass<UVoraxiaCameraOcclusionDitherComponent>())
			{
				DitherComponent->ApplySettings(SettingsAsset->OcclusionDitherSettings);
			}
		}
	}

	AssignedSettingsAsset = SettingsAsset;
	OnCameraSettingsApplied.Broadcast(SettingsAsset);

	UE_LOG(
		LogVoraxiaCamera,
		Log,
		TEXT("Voraxia camera settings smooth transition started from '%s' over %.2fs."),
		*GetNameSafe(SettingsAsset),
		SafeBlendTime
	);

	return true;
}

void UVoraxiaCameraComponent::CaptureCurrentSettingsToAssignedAsset()
{
	CaptureCurrentSettingsToAsset(AssignedSettingsAsset);
}

void UVoraxiaCameraComponent::ApplyAssignedSettingsAsset()
{
	ApplyCameraSettingsAsset(AssignedSettingsAsset);
}

bool UVoraxiaCameraComponent::ApplyAssignedSettingsAssetSmooth(
	const float BlendTime,
	UCurveFloat* BlendCurve
)
{
	return ApplyCameraSettingsAssetSmooth(
		AssignedSettingsAsset,
		BlendTime,
		BlendCurve
	);
}

void UVoraxiaCameraComponent::PopulatePersistentSettings(
	FVoraxiaCameraPersistentSettings& OutSettings
) const
{
	OutSettings.bAutoFindCameraComponent = bAutoFindCameraComponent;
	OutSettings.CameraDistance = CameraDistance;
	OutSettings.PivotHeight = PivotHeight;
	OutSettings.AdditionalCameraOffset = AdditionalCameraOffset;
	OutSettings.ShoulderOffset = ShoulderOffset;
	OutSettings.bUseRightShoulder = bUseRightShoulder;
	OutSettings.AdditionalPivotOffset = AdditionalPivotOffset;
	OutSettings.bClampCameraOffsetWithinDistance = bClampCameraOffsetWithinDistance;
	OutSettings.bUseTargetCameraFOVAsBase = bUseTargetCameraFOVAsBase;
	OutSettings.BaseFOV = BaseFOV;
	OutSettings.MinFOV = MinFOV;
	OutSettings.MaxFOV = MaxFOV;
	OutSettings.InitialPitch = InitialPitch;
	OutSettings.MinPitch = MinPitch;
	OutSettings.MaxPitch = MaxPitch;
	OutSettings.YawInputSpeed = YawInputSpeed;
	OutSettings.PitchInputSpeed = PitchInputSpeed;
	OutSettings.bInvertPitchInput = bInvertPitchInput;
	OutSettings.bEnableRotationLag = bEnableRotationLag;
	OutSettings.RotationLagSpeed = RotationLagSpeed;
	OutSettings.LookInputDeadZone = LookInputDeadZone;
	OutSettings.bEnableYawConstraints = bEnableYawConstraints;
	OutSettings.YawConstraintSoftZone = YawConstraintSoftZone;
	OutSettings.bDrawYawConstraintDebug = bDrawYawConstraintDebug;
	OutSettings.YawConstraintDebugDrawLength = YawConstraintDebugDrawLength;
	OutSettings.bEnableCameraCollision = bEnableCameraCollision;
	OutSettings.CameraCollisionChannel = CameraCollisionChannel;
	OutSettings.CameraCollisionRadius = CameraCollisionRadius;
	OutSettings.MinDistanceFromPivot = MinDistanceFromPivot;
	OutSettings.CollisionProbeStartOffset = CollisionProbeStartOffset;
	OutSettings.CollisionSafetyPadding = CollisionSafetyPadding;
	OutSettings.CollisionCompressionSpeed = CollisionCompressionSpeed;
	OutSettings.CollisionRecoverySpeed = CollisionRecoverySpeed;
	OutSettings.bEnablePredictiveAvoidance = bEnablePredictiveAvoidance;
	OutSettings.FeelerYawOffset = FeelerYawOffset;
	OutSettings.FeelerPairs = FeelerPairs;
	OutSettings.FeelerProbeRadius = FeelerProbeRadius;
	OutSettings.bEnableMovementAnticipation = bEnableMovementAnticipation;
	OutSettings.MovementAnticipationForwardOffset = MovementAnticipationForwardOffset;
	OutSettings.MovementAnticipationBackwardOffset = MovementAnticipationBackwardOffset;
	OutSettings.MovementAnticipationSideOffset = MovementAnticipationSideOffset;
	OutSettings.MovementAnticipationFullSpeed = MovementAnticipationFullSpeed;
	OutSettings.MovementAnticipationInterpSpeed = MovementAnticipationInterpSpeed;
	OutSettings.MovementAnticipationReturnSpeed = MovementAnticipationReturnSpeed;
	OutSettings.MovementAnticipationDeadZone = MovementAnticipationDeadZone;
	OutSettings.bReduceMovementAnticipationWhileLooking = bReduceMovementAnticipationWhileLooking;
	OutSettings.LookInputAnticipationMultiplier = LookInputAnticipationMultiplier;
	OutSettings.MovementAnticipationLookInputThreshold = MovementAnticipationLookInputThreshold;
	OutSettings.bEnablePitchDistanceModifier = bEnablePitchDistanceModifier;
	OutSettings.PitchDistanceAtMinPitchOffset = PitchDistanceAtMinPitchOffset;
	OutSettings.PitchDistanceAtMaxPitchOffset = PitchDistanceAtMaxPitchOffset;
	OutSettings.bEnablePitchFOVModifier = bEnablePitchFOVModifier;
	OutSettings.PitchFOVAtMinPitchOffset = PitchFOVAtMinPitchOffset;
	OutSettings.PitchFOVAtMaxPitchOffset = PitchFOVAtMaxPitchOffset;
	OutSettings.PitchModifierInterpSpeed = PitchModifierInterpSpeed;
	OutSettings.bEnableSpeedDistanceModifier = bEnableSpeedDistanceModifier;
	OutSettings.SpeedDistanceOffset = SpeedDistanceOffset;
	OutSettings.bEnableSpeedFOVModifier = bEnableSpeedFOVModifier;
	OutSettings.SpeedFOVOffset = SpeedFOVOffset;
	OutSettings.SpeedModifierFullSpeed = SpeedModifierFullSpeed;
	OutSettings.SpeedModifierInterpSpeed = SpeedModifierInterpSpeed;
	OutSettings.bEnableFocusSystem = bEnableFocusSystem;
	OutSettings.DefaultFocusActorTag = DefaultFocusActorTag;
	OutSettings.DefaultFocusBlendInTime = DefaultFocusBlendInTime;
	OutSettings.DefaultFocusBlendOutTime = DefaultFocusBlendOutTime;
	OutSettings.bClampFocusPitchToCameraLimits = bClampFocusPitchToCameraLimits;
	OutSettings.FocusMinimumTargetDistance = FocusMinimumTargetDistance;
	OutSettings.FocusTargetSearchDistance = FocusTargetSearchDistance;
	OutSettings.bRequireFocusTargetInFront = bRequireFocusTargetInFront;
	OutSettings.FocusTargetMinForwardDot = FocusTargetMinForwardDot;
	OutSettings.bRequireFocusTargetLineOfSight = bRequireFocusTargetLineOfSight;
	OutSettings.FocusTargetLineOfSightChannel = FocusTargetLineOfSightChannel;
	OutSettings.FocusTargetAlignmentScoreWeight = FocusTargetAlignmentScoreWeight;
	OutSettings.FocusTargetDistanceScoreWeight = FocusTargetDistanceScoreWeight;
	OutSettings.bLogFocusTargetSelection = bLogFocusTargetSelection;
	OutSettings.bDrawFocusTargetSelectionDebug = bDrawFocusTargetSelectionDebug;
	OutSettings.FocusTargetSelectionDebugDuration = FocusTargetSelectionDebugDuration;
	OutSettings.bDrawFocusDebug = bDrawFocusDebug;
	OutSettings.bDrawCameraCollisionDebug = bDrawCameraCollisionDebug;
	OutSettings.bDrawCameraStateDebug = bDrawCameraStateDebug;
	OutSettings.bShowSlateDebugPanel = bShowSlateDebugPanel;
	OutSettings.SlateDebugPanelWidth = SlateDebugPanelWidth;
	OutSettings.SlateDebugPanelZOrder = SlateDebugPanelZOrder;
	OutSettings.bEnablePitchConstraints = bEnablePitchConstraints;
	OutSettings.PitchConstraintTolerance = PitchConstraintTolerance;
	OutSettings.bEnablePitchMovementFollow = bEnablePitchMovementFollow;
	OutSettings.RestingCameraPitch = RestingCameraPitch;
	OutSettings.PitchFollowSpeed = PitchFollowSpeed;
	OutSettings.PitchFollowTimeThreshold = PitchFollowTimeThreshold;
	OutSettings.PitchFollowAngleThreshold = PitchFollowAngleThreshold;
	OutSettings.PitchFollowMinSpeedThreshold = PitchFollowMinSpeedThreshold;
	OutSettings.bEnableYawMovementFollow = bEnableYawMovementFollow;
	OutSettings.YawFollowSpeed = YawFollowSpeed;
	OutSettings.YawFollowTimeThreshold = YawFollowTimeThreshold;
	OutSettings.YawFollowAngleThreshold = YawFollowAngleThreshold;
	OutSettings.YawFollowMinSpeedThreshold = YawFollowMinSpeedThreshold;
	OutSettings.bYawFollowOnlyForwardMovement = bYawFollowOnlyForwardMovement;
	OutSettings.bEnableIdleCameraShake = bEnableIdleCameraShake;
	OutSettings.IdleCameraShakeClass = IdleCameraShakeClass;
	OutSettings.IdleCameraShakeScale = IdleCameraShakeScale;
	OutSettings.IdleCameraShakeMaxSpeed = IdleCameraShakeMaxSpeed;
	OutSettings.bEnableMovementCameraShake = bEnableMovementCameraShake;
	OutSettings.MovementCameraShakeClass = MovementCameraShakeClass;
	OutSettings.MovementCameraShakeMinSpeed = MovementCameraShakeMinSpeed;
	OutSettings.MovementCameraShakeMaxSpeed = MovementCameraShakeMaxSpeed;
	OutSettings.MovementCameraShakeMinScale = MovementCameraShakeMinScale;
	OutSettings.MovementCameraShakeMaxScale = MovementCameraShakeMaxScale;
	OutSettings.MovementCameraShakeScaleInterpSpeed = MovementCameraShakeScaleInterpSpeed;
}

void UVoraxiaCameraComponent::ApplyPersistentSettings(
	const FVoraxiaCameraPersistentSettings& InSettings,
	const bool bResetTransientState
)
{
	bAutoFindCameraComponent = InSettings.bAutoFindCameraComponent;
	CameraDistance = InSettings.CameraDistance;
	PivotHeight = InSettings.PivotHeight;
	AdditionalCameraOffset = InSettings.AdditionalCameraOffset;
	ShoulderOffset = FMath::Max(0.0f, InSettings.ShoulderOffset);
	bUseRightShoulder = InSettings.bUseRightShoulder;
	AdditionalPivotOffset = InSettings.AdditionalPivotOffset;
	bClampCameraOffsetWithinDistance = InSettings.bClampCameraOffsetWithinDistance;
	bUseTargetCameraFOVAsBase = InSettings.bUseTargetCameraFOVAsBase;
	BaseFOV = InSettings.BaseFOV;
	MinFOV = InSettings.MinFOV;
	MaxFOV = InSettings.MaxFOV;
	InitialPitch = InSettings.InitialPitch;
	MinPitch = InSettings.MinPitch;
	MaxPitch = InSettings.MaxPitch;
	YawInputSpeed = InSettings.YawInputSpeed;
	PitchInputSpeed = InSettings.PitchInputSpeed;
	bInvertPitchInput = InSettings.bInvertPitchInput;
	bEnableRotationLag = InSettings.bEnableRotationLag;
	RotationLagSpeed = InSettings.RotationLagSpeed;
	LookInputDeadZone = InSettings.LookInputDeadZone;
	bEnableYawConstraints = InSettings.bEnableYawConstraints;
	YawConstraintSoftZone = FMath::Max(0.0f, InSettings.YawConstraintSoftZone);
	bDrawYawConstraintDebug = InSettings.bDrawYawConstraintDebug;
	YawConstraintDebugDrawLength = FMath::Max(10.0f, InSettings.YawConstraintDebugDrawLength);
	bEnableCameraCollision = InSettings.bEnableCameraCollision;
	CameraCollisionChannel = InSettings.CameraCollisionChannel;
	CameraCollisionRadius = InSettings.CameraCollisionRadius;
	MinDistanceFromPivot = InSettings.MinDistanceFromPivot;
	CollisionProbeStartOffset = InSettings.CollisionProbeStartOffset;
	CollisionSafetyPadding = InSettings.CollisionSafetyPadding;
	CollisionCompressionSpeed = InSettings.CollisionCompressionSpeed;
	CollisionRecoverySpeed = InSettings.CollisionRecoverySpeed;
	bEnablePredictiveAvoidance = InSettings.bEnablePredictiveAvoidance;
	FeelerYawOffset = InSettings.FeelerYawOffset;
	FeelerPairs = InSettings.FeelerPairs;
	FeelerProbeRadius = InSettings.FeelerProbeRadius;
	bEnableMovementAnticipation = InSettings.bEnableMovementAnticipation;
	MovementAnticipationForwardOffset = InSettings.MovementAnticipationForwardOffset;
	MovementAnticipationBackwardOffset = InSettings.MovementAnticipationBackwardOffset;
	MovementAnticipationSideOffset = InSettings.MovementAnticipationSideOffset;
	MovementAnticipationFullSpeed = InSettings.MovementAnticipationFullSpeed;
	MovementAnticipationInterpSpeed = InSettings.MovementAnticipationInterpSpeed;
	MovementAnticipationReturnSpeed = InSettings.MovementAnticipationReturnSpeed;
	MovementAnticipationDeadZone = InSettings.MovementAnticipationDeadZone;
	bReduceMovementAnticipationWhileLooking = InSettings.bReduceMovementAnticipationWhileLooking;
	LookInputAnticipationMultiplier = InSettings.LookInputAnticipationMultiplier;
	MovementAnticipationLookInputThreshold = InSettings.MovementAnticipationLookInputThreshold;
	bEnablePitchDistanceModifier = InSettings.bEnablePitchDistanceModifier;
	PitchDistanceAtMinPitchOffset = InSettings.PitchDistanceAtMinPitchOffset;
	PitchDistanceAtMaxPitchOffset = InSettings.PitchDistanceAtMaxPitchOffset;
	bEnablePitchFOVModifier = InSettings.bEnablePitchFOVModifier;
	PitchFOVAtMinPitchOffset = InSettings.PitchFOVAtMinPitchOffset;
	PitchFOVAtMaxPitchOffset = InSettings.PitchFOVAtMaxPitchOffset;
	PitchModifierInterpSpeed = InSettings.PitchModifierInterpSpeed;
	bEnableSpeedDistanceModifier = InSettings.bEnableSpeedDistanceModifier;
	SpeedDistanceOffset = InSettings.SpeedDistanceOffset;
	bEnableSpeedFOVModifier = InSettings.bEnableSpeedFOVModifier;
	SpeedFOVOffset = InSettings.SpeedFOVOffset;
	SpeedModifierFullSpeed = InSettings.SpeedModifierFullSpeed;
	SpeedModifierInterpSpeed = InSettings.SpeedModifierInterpSpeed;
	bEnableFocusSystem = InSettings.bEnableFocusSystem;
	DefaultFocusActorTag = InSettings.DefaultFocusActorTag;
	DefaultFocusBlendInTime = InSettings.DefaultFocusBlendInTime;
	DefaultFocusBlendOutTime = InSettings.DefaultFocusBlendOutTime;
	bClampFocusPitchToCameraLimits = InSettings.bClampFocusPitchToCameraLimits;
	FocusMinimumTargetDistance = InSettings.FocusMinimumTargetDistance;
	FocusTargetSearchDistance = InSettings.FocusTargetSearchDistance;
	bRequireFocusTargetInFront = InSettings.bRequireFocusTargetInFront;
	FocusTargetMinForwardDot = InSettings.FocusTargetMinForwardDot;
	bRequireFocusTargetLineOfSight = InSettings.bRequireFocusTargetLineOfSight;
	FocusTargetLineOfSightChannel = InSettings.FocusTargetLineOfSightChannel;
	FocusTargetAlignmentScoreWeight = InSettings.FocusTargetAlignmentScoreWeight;
	FocusTargetDistanceScoreWeight = InSettings.FocusTargetDistanceScoreWeight;
	bLogFocusTargetSelection = InSettings.bLogFocusTargetSelection;
	bDrawFocusTargetSelectionDebug = InSettings.bDrawFocusTargetSelectionDebug;
	FocusTargetSelectionDebugDuration = InSettings.FocusTargetSelectionDebugDuration;
	bDrawFocusDebug = InSettings.bDrawFocusDebug;
	bDrawCameraCollisionDebug = InSettings.bDrawCameraCollisionDebug;
	bDrawCameraStateDebug = InSettings.bDrawCameraStateDebug;
	bShowSlateDebugPanel = InSettings.bShowSlateDebugPanel;
	SlateDebugPanelWidth = InSettings.SlateDebugPanelWidth;
	SlateDebugPanelZOrder = InSettings.SlateDebugPanelZOrder;
	bEnablePitchConstraints = InSettings.bEnablePitchConstraints;
	PitchConstraintTolerance = InSettings.PitchConstraintTolerance;
	bEnablePitchMovementFollow = InSettings.bEnablePitchMovementFollow;
	RestingCameraPitch = InSettings.RestingCameraPitch;
	PitchFollowSpeed = InSettings.PitchFollowSpeed;
	PitchFollowTimeThreshold = InSettings.PitchFollowTimeThreshold;
	PitchFollowAngleThreshold = InSettings.PitchFollowAngleThreshold;
	PitchFollowMinSpeedThreshold = InSettings.PitchFollowMinSpeedThreshold;
	bEnableYawMovementFollow = InSettings.bEnableYawMovementFollow;
	YawFollowSpeed = InSettings.YawFollowSpeed;
	YawFollowTimeThreshold = InSettings.YawFollowTimeThreshold;
	YawFollowAngleThreshold = InSettings.YawFollowAngleThreshold;
	YawFollowMinSpeedThreshold = InSettings.YawFollowMinSpeedThreshold;
	bYawFollowOnlyForwardMovement = InSettings.bYawFollowOnlyForwardMovement;
	bEnableIdleCameraShake = InSettings.bEnableIdleCameraShake;
	IdleCameraShakeClass = InSettings.IdleCameraShakeClass;
	IdleCameraShakeScale = InSettings.IdleCameraShakeScale;
	IdleCameraShakeMaxSpeed = InSettings.IdleCameraShakeMaxSpeed;
	bEnableMovementCameraShake = InSettings.bEnableMovementCameraShake;
	MovementCameraShakeClass = InSettings.MovementCameraShakeClass;
	MovementCameraShakeMinSpeed = InSettings.MovementCameraShakeMinSpeed;
	MovementCameraShakeMaxSpeed = InSettings.MovementCameraShakeMaxSpeed;
	MovementCameraShakeMinScale = InSettings.MovementCameraShakeMinScale;
	MovementCameraShakeMaxScale = InSettings.MovementCameraShakeMaxScale;
	MovementCameraShakeScaleInterpSpeed = InSettings.MovementCameraShakeScaleInterpSpeed;

	if (bResetTransientState)
	{
		ResetTransientStateAfterSettingsApply();
	}
}

void UVoraxiaCameraComponent::ResetTransientStateAfterSettingsApply()
{
	StopAllCameraShakes(true);
	IdleCameraShakeHandle = FVoraxiaCameraShakeHandle();
	MovementCameraShakeHandle = FVoraxiaCameraShakeHandle();
	CurrentMovementCameraShakeScale = 0.0f;

	InitializeRuntimeState();
	CurrentCollisionDistanceFromPivot = -1.0f;
	bWasCameraCollisionBlocked = false;
	LastDesiredDistanceFromPivot = 0.0f;
	LastEffectiveDistanceFromPivot = 0.0f;
	CurrentPitchDistanceOffset = 0.0f;
	CurrentPitchFOVOffset = 0.0f;
	CurrentSpeedDistanceOffset = 0.0f;
	CurrentSpeedFOVOffset = 0.0f;
	CurrentMovementAnticipationOffset = FVector::ZeroVector;

	ResetYawConstraintState();

	FocusAlpha = 0.0f;
	ResetFocusTarget();

	if (bShowSlateDebugPanel)
	{
		CreateSlateDebugPanel();
	}
	else
	{
		DestroySlateDebugPanel();
	}
}

float UVoraxiaCameraComponent::GetTargetShoulderOffset() const
{
	const float SafeShoulderOffset = FMath::Max(0.0f, ShoulderOffset);

	return bUseRightShoulder
		? SafeShoulderOffset
		: -SafeShoulderOffset;
}

void UVoraxiaCameraComponent::InitializeRuntimeState()
{
	RuntimeCameraDistance.Initialize(CameraDistance);
	RuntimePivotHeight.Initialize(PivotHeight);
	RuntimeFOVOffset.Initialize(0.0f);
	RuntimeShoulderOffset.Initialize(GetTargetShoulderOffset());

	RuntimeCameraOffset.Initialize(FVector::ZeroVector);
	RuntimePivotOffset.Initialize(FVector::ZeroVector);
}

void UVoraxiaCameraComponent::UpdateRuntimeState(const float DeltaTime)
{
	RuntimeCameraDistance.Tick(DeltaTime);
	RuntimePivotHeight.Tick(DeltaTime);
	RuntimeFOVOffset.Tick(DeltaTime);
	RuntimeShoulderOffset.Tick(DeltaTime);

	RuntimeCameraOffset.Tick(DeltaTime);
	RuntimePivotOffset.Tick(DeltaTime);
}


void UVoraxiaCameraComponent::ResetYawConstraintState()
{
	bYawConstraintTargetActive = false;

	CurrentYawConstraintReferenceYaw = FMath::UnwindDegrees(DesiredRotation.Yaw);
	CurrentYawConstraintMinDelta = -180.0f;
	CurrentYawConstraintMaxDelta = 180.0f;
	CurrentYawConstraintAlpha = 0.0f;

	YawConstraintStartReferenceYaw = CurrentYawConstraintReferenceYaw;
	YawConstraintStartMinDelta = CurrentYawConstraintMinDelta;
	YawConstraintStartMaxDelta = CurrentYawConstraintMaxDelta;
	YawConstraintStartAlpha = CurrentYawConstraintAlpha;

	YawConstraintTargetReferenceYaw = CurrentYawConstraintReferenceYaw;
	YawConstraintTargetMinDelta = CurrentYawConstraintMinDelta;
	YawConstraintTargetMaxDelta = CurrentYawConstraintMaxDelta;
	YawConstraintTargetAlpha = CurrentYawConstraintAlpha;

	YawConstraintBlendTime = 0.0f;
	YawConstraintBlendElapsed = 0.0f;
}

void UVoraxiaCameraComponent::BeginYawConstraintBlend(
	const float ReferenceYaw,
	const float MinYawDelta,
	const float MaxYawDelta,
	const float TargetAlpha,
	const float BlendTime
)
{
	YawConstraintStartReferenceYaw = CurrentYawConstraintReferenceYaw;
	YawConstraintStartMinDelta = CurrentYawConstraintMinDelta;
	YawConstraintStartMaxDelta = CurrentYawConstraintMaxDelta;
	YawConstraintStartAlpha = CurrentYawConstraintAlpha;

	YawConstraintTargetReferenceYaw = FMath::UnwindDegrees(ReferenceYaw);
	YawConstraintTargetMinDelta = FMath::Clamp(
		FMath::Min(MinYawDelta, MaxYawDelta),
		-180.0f,
		180.0f
	);
	YawConstraintTargetMaxDelta = FMath::Clamp(
		FMath::Max(MinYawDelta, MaxYawDelta),
		-180.0f,
		180.0f
	);
	YawConstraintTargetAlpha = FMath::Clamp(TargetAlpha, 0.0f, 1.0f);

	YawConstraintBlendTime = FMath::Max(0.0f, BlendTime);
	YawConstraintBlendElapsed = 0.0f;
	bYawConstraintTargetActive = YawConstraintTargetAlpha > KINDA_SMALL_NUMBER;

	if (YawConstraintBlendTime <= KINDA_SMALL_NUMBER)
	{
		CurrentYawConstraintReferenceYaw = YawConstraintTargetReferenceYaw;
		CurrentYawConstraintMinDelta = YawConstraintTargetMinDelta;
		CurrentYawConstraintMaxDelta = YawConstraintTargetMaxDelta;
		CurrentYawConstraintAlpha = YawConstraintTargetAlpha;

		if (!bYawConstraintTargetActive)
		{
			CurrentYawConstraintMinDelta = -180.0f;
			CurrentYawConstraintMaxDelta = 180.0f;
		}
	}
}

void UVoraxiaCameraComponent::UpdateYawConstraintState(const float DeltaTime)
{
	if (YawConstraintBlendTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	YawConstraintBlendElapsed += DeltaTime;

	const float RawAlpha = FMath::Clamp(
		YawConstraintBlendElapsed / YawConstraintBlendTime,
		0.0f,
		1.0f
	);
	const float SmoothAlpha = RawAlpha * RawAlpha * (3.0f - 2.0f * RawAlpha);

	const float ReferenceYawDelta = FMath::FindDeltaAngleDegrees(
		YawConstraintStartReferenceYaw,
		YawConstraintTargetReferenceYaw
	);

	CurrentYawConstraintReferenceYaw = FMath::UnwindDegrees(
		YawConstraintStartReferenceYaw + ReferenceYawDelta * SmoothAlpha
	);
	CurrentYawConstraintMinDelta = FMath::Lerp(
		YawConstraintStartMinDelta,
		YawConstraintTargetMinDelta,
		SmoothAlpha
	);
	CurrentYawConstraintMaxDelta = FMath::Lerp(
		YawConstraintStartMaxDelta,
		YawConstraintTargetMaxDelta,
		SmoothAlpha
	);
	CurrentYawConstraintAlpha = FMath::Lerp(
		YawConstraintStartAlpha,
		YawConstraintTargetAlpha,
		SmoothAlpha
	);

	if (RawAlpha >= 1.0f)
	{
		YawConstraintBlendTime = 0.0f;
		YawConstraintBlendElapsed = 0.0f;

		if (!bYawConstraintTargetActive)
		{
			CurrentYawConstraintMinDelta = -180.0f;
			CurrentYawConstraintMaxDelta = 180.0f;
			CurrentYawConstraintAlpha = 0.0f;
		}
	}
}

float UVoraxiaCameraComponent::GetYawConstraintSoftZone() const
{
	const float ConstraintWidth = FMath::Max(
		0.0f,
		CurrentYawConstraintMaxDelta - CurrentYawConstraintMinDelta
	);

	return FMath::Min(
		FMath::Max(0.0f, YawConstraintSoftZone),
		ConstraintWidth * 0.5f
	);
}

float UVoraxiaCameraComponent::ClampYawToConstraint(const float Yaw) const
{
	if (!bEnableYawConstraints || CurrentYawConstraintAlpha <= KINDA_SMALL_NUMBER)
	{
		return FMath::UnwindDegrees(Yaw);
	}

	const float YawDelta = FMath::FindDeltaAngleDegrees(
		CurrentYawConstraintReferenceYaw,
		Yaw
	);
	const float ClampedYawDelta = FMath::Clamp(
		YawDelta,
		CurrentYawConstraintMinDelta,
		CurrentYawConstraintMaxDelta
	);

	return FMath::UnwindDegrees(
		CurrentYawConstraintReferenceYaw + ClampedYawDelta
	);
}

void UVoraxiaCameraComponent::ApplyYawInputWithConstraints(
	const float YawInputDelta
)
{
	if (!bEnableYawConstraints || CurrentYawConstraintAlpha <= KINDA_SMALL_NUMBER)
	{
		DesiredRotation.Yaw = FMath::UnwindDegrees(
			DesiredRotation.Yaw + YawInputDelta
		);
		return;
	}

	const float CurrentYawDelta = FMath::FindDeltaAngleDegrees(
		CurrentYawConstraintReferenceYaw,
		DesiredRotation.Yaw
	);

	float ConstraintScale = 1.0f;
	const float SoftZone = GetYawConstraintSoftZone();

	if (SoftZone > KINDA_SMALL_NUMBER)
	{
		if (YawInputDelta < 0.0f)
		{
			const float DistanceToMin = FMath::Max(
				0.0f,
				CurrentYawDelta - CurrentYawConstraintMinDelta
			);

			if (DistanceToMin < SoftZone)
			{
				ConstraintScale = FMath::Clamp(
					DistanceToMin / SoftZone,
					0.0f,
					1.0f
				);
			}
		}
		else if (YawInputDelta > 0.0f)
		{
			const float DistanceToMax = FMath::Max(
				0.0f,
				CurrentYawConstraintMaxDelta - CurrentYawDelta
			);

			if (DistanceToMax < SoftZone)
			{
				ConstraintScale = FMath::Clamp(
					DistanceToMax / SoftZone,
					0.0f,
					1.0f
				);
			}
		}

		ConstraintScale = ConstraintScale * ConstraintScale * (
			3.0f - 2.0f * ConstraintScale
		);
	}

	DesiredRotation.Yaw = ClampYawToConstraint(
		DesiredRotation.Yaw + YawInputDelta * ConstraintScale
	);
}

void UVoraxiaCameraComponent::ApplyYawConstraints()
{
	if (!bEnableYawConstraints || CurrentYawConstraintAlpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	DesiredRotation.Yaw = ClampYawToConstraint(DesiredRotation.Yaw);
}

void UVoraxiaCameraComponent::DrawYawConstraintDebug() const
{
	if (!bDrawYawConstraintDebug
		|| !bEnableYawConstraints
		|| CurrentYawConstraintAlpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const FVector PivotLocation = CalculatePivotLocation();
	const float DebugLength = FMath::Max(10.0f, YawConstraintDebugDrawLength);

	const float MinWorldYaw = CurrentYawConstraintReferenceYaw + CurrentYawConstraintMinDelta;
	const float MaxWorldYaw = CurrentYawConstraintReferenceYaw + CurrentYawConstraintMaxDelta;

	const FVector ReferenceDirection = FRotator(
		0.0f,
		CurrentYawConstraintReferenceYaw,
		0.0f
	).Vector();

	const FVector MinDirection = FRotator(0.0f, MinWorldYaw, 0.0f).Vector();
	const FVector MaxDirection = FRotator(0.0f, MaxWorldYaw, 0.0f).Vector();

	DrawDebugDirectionalArrow(
		World,
		PivotLocation,
		PivotLocation + ReferenceDirection * DebugLength,
		24.0f,
		FColor::Cyan,
		false,
		0.0f,
		0,
		2.0f
	);

	DrawDebugDirectionalArrow(
		World,
		PivotLocation,
		PivotLocation + MinDirection * DebugLength,
		18.0f,
		FColor::Red,
		false,
		0.0f,
		0,
		1.5f
	);

	DrawDebugDirectionalArrow(
		World,
		PivotLocation,
		PivotLocation + MaxDirection * DebugLength,
		18.0f,
		FColor::Red,
		false,
		0.0f,
		0,
		1.5f
	);

	DrawDebugString(
		World,
		PivotLocation + FVector(0.0f, 0.0f, 56.0f),
		FString::Printf(
			TEXT("Yaw Constraint %.0f%% | Ref %.1f | [%.1f, %.1f]"),
			CurrentYawConstraintAlpha * 100.0f,
			CurrentYawConstraintReferenceYaw,
			CurrentYawConstraintMinDelta,
			CurrentYawConstraintMaxDelta
		),
		nullptr,
		FColor::Cyan,
		0.0f,
		true
	);
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
	
	DesiredRotation = UnfocusedDesiredRotation;
	
	if (bHasYawInput)
	{
		ApplyYawInputWithConstraints(
			PendingYawInput * YawInputSpeed * DeltaTime
		);
	}

	if (bHasPitchInput)
	{
		const float PitchInputDelta = PendingPitchInput * PitchInputSpeed * DeltaTime;
		ApplyPitchInputWithConstraints(PitchInputDelta);
	}

	UpdatePitchFollow(DeltaTime);
	UpdateYawFollow(DeltaTime);
	ApplyYawConstraints();

	DesiredRotation.Roll = 0.0f;

	if (bEnablePitchConstraints)
	{
		DesiredRotation.Pitch = FMath::Clamp(DesiredRotation.Pitch, MinPitch, MaxPitch);
	}

	UnfocusedDesiredRotation = DesiredRotation;

	UpdateFocus(DeltaTime);

	DrawYawConstraintDebug();

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

void UVoraxiaCameraComponent::FocusDefaultTaggedActor(const float BlendTime)
{
	if (!bEnableFocusSystem)
	{
		return;
	}

	if (DefaultFocusActorTag.IsNone())
	{
		UE_LOG(
			LogVoraxiaCamera,
			Warning,
			TEXT("Voraxia camera has no DefaultFocusActorTag assigned.")
		);

		return;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();

	if (!World || !Owner)
	{
		return;
	}

	const FVector SearchOrigin = Owner->GetActorLocation();
	const FVector TraceStart = TargetCamera
		? TargetCamera->GetComponentLocation()
		: CalculatePivotLocation();

	const FVector ViewForward = TargetCamera
		? TargetCamera->GetForwardVector().GetSafeNormal()
		: SmoothedRotation.Vector().GetSafeNormal();

	const float MaxDistanceSquared = FocusTargetSearchDistance > 0.0f
		? FMath::Square(FocusTargetSearchDistance)
		: TNumericLimits<float>::Max();

	AActor* BestActor = nullptr;
	FVector BestFocusLocation = FVector::ZeroVector;
	float BestScore = TNumericLimits<float>::Max();

	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;

		if (!Actor || Actor == Owner || !Actor->ActorHasTag(DefaultFocusActorTag))
		{
			continue;
		}

		FVector FocusLocation = Actor->GetActorLocation();

		if (Actor->GetClass()->ImplementsInterface(UVoraxiaFocusableInterface::StaticClass()))
		{
			if (!IVoraxiaFocusableInterface::Execute_CanBeFocused(Actor, Owner))
			{
				if (bLogFocusTargetSelection)
				{
					UE_LOG(
						LogVoraxiaCamera,
						Log,
						TEXT("Voraxia focus selection rejected '%s': CanBeFocused returned false."),
						*GetNameSafe(Actor)
					);
				}

				DrawFocusTargetSelectionDebug(
					TraceStart,
					FocusLocation,
					FColor::Red,
					TEXT("Rejected: Not Focusable")
				);

				continue;
			}

			FocusLocation = IVoraxiaFocusableInterface::Execute_GetFocusLocation(Actor, Owner);
		}

		const FVector ToActor = FocusLocation - SearchOrigin;
		const float DistanceSquared = ToActor.SizeSquared();

		if (DistanceSquared > MaxDistanceSquared)
		{
			if (bLogFocusTargetSelection)
			{
				UE_LOG(
					LogVoraxiaCamera,
					Log,
					TEXT("Voraxia focus selection rejected '%s': %.1f units exceeds search distance %.1f."),
					*GetNameSafe(Actor),
					FMath::Sqrt(DistanceSquared),
					FocusTargetSearchDistance
				);
			}

			DrawFocusTargetSelectionDebug(
				TraceStart,
				FocusLocation,
				FColor::Red,
				TEXT("Rejected: Out Of Range")
			);

			continue;
		}

		const FVector DirectionToActor = ToActor.GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(ViewForward, DirectionToActor);

		if (bRequireFocusTargetInFront && ForwardDot < FocusTargetMinForwardDot)
		{
			if (bLogFocusTargetSelection)
			{
				UE_LOG(
					LogVoraxiaCamera,
					Log,
					TEXT("Voraxia focus selection rejected '%s': forward dot %.3f is below threshold %.3f."),
					*GetNameSafe(Actor),
					ForwardDot,
					FocusTargetMinForwardDot
				);
			}

			DrawFocusTargetSelectionDebug(
				TraceStart,
				FocusLocation,
				FColor::Red,
				TEXT("Rejected: Outside View")
			);

			continue;
		}

		FHitResult BlockingHit;

		if (bRequireFocusTargetLineOfSight && !HasFocusTargetLineOfSight(
			Actor,
			TraceStart,
			FocusLocation,
			&BlockingHit
		))
		{
			if (bLogFocusTargetSelection)
			{
				UE_LOG(
					LogVoraxiaCamera,
					Log,
					TEXT("Voraxia focus selection rejected '%s': line of sight blocked by '%s'."),
					*GetNameSafe(Actor),
					*GetNameSafe(BlockingHit.GetActor())
				);
			}

			DrawFocusTargetSelectionDebug(
				TraceStart,
				FocusLocation,
				FColor::Red,
				TEXT("Rejected: Blocked")
			);

			continue;
		}

		const float Score = CalculateFocusTargetSelectionScore(ForwardDot, DistanceSquared);

		if (bLogFocusTargetSelection)
		{
			UE_LOG(
				LogVoraxiaCamera,
				Log,
				TEXT("Voraxia focus selection accepted '%s': dot %.3f, distance %.1f, score %.2f."),
				*GetNameSafe(Actor),
				ForwardDot,
				FMath::Sqrt(DistanceSquared),
				Score
			);
		}

		DrawFocusTargetSelectionDebug(
			TraceStart,
			FocusLocation,
			FColor::Yellow,
			TEXT("Candidate")
		);

		if (Score < BestScore)
		{
			BestScore = Score;
			BestActor = Actor;
			BestFocusLocation = FocusLocation;
		}
	}

	if (!BestActor)
	{
	UE_LOG(
			LogVoraxiaCamera,
			Warning,
			TEXT("Voraxia camera could not find a valid focus actor with tag '%s'."),
			*DefaultFocusActorTag.ToString()
		);

		return;
	}

	SetFocusActor(
		BestActor,
		BlendTime >= 0.0f ? BlendTime : DefaultFocusBlendInTime
	);

	DrawFocusTargetSelectionDebug(
		TraceStart,
		BestFocusLocation,
		FColor::Green,
		TEXT("Selected")
	);

	UE_LOG(
		LogVoraxiaCamera,
		Log,
		TEXT("Voraxia camera focused tagged actor '%s' with tag '%s'. Score: %.2f."),
		*GetNameSafe(BestActor),
		*DefaultFocusActorTag.ToString(),
		BestScore
	);
}

bool UVoraxiaCameraComponent::HasFocusTargetLineOfSight(
	const AActor* CandidateActor,
	const FVector& TraceStart,
	const FVector& FocusLocation,
	FHitResult* OutBlockingHit
) const
{
	if (!bRequireFocusTargetLineOfSight)
	{
		return true;
	}

	UWorld* World = GetWorld();

	if (!World || !CandidateActor)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(VoraxiaFocusTargetLineOfSight),
		false
	);

	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		FocusLocation,
		FocusTargetLineOfSightChannel,
		QueryParams
	);

	if (!bHit)
	{
		return true;
	}

	const bool bHitCandidateActor = Hit.GetActor() == CandidateActor;
	const bool bHitCandidateComponent = Hit.GetComponent()
		&& Hit.GetComponent()->GetOwner() == CandidateActor;

	if (bHitCandidateActor || bHitCandidateComponent)
	{
		return true;
	}

	if (OutBlockingHit)
	{
		*OutBlockingHit = Hit;
	}

	return false;
}

float UVoraxiaCameraComponent::CalculateFocusTargetSelectionScore(
	const float ForwardDot,
	const float DistanceSquared
) const
{
	const float AlignmentPenalty = 1.0f - FMath::Clamp(ForwardDot, -1.0f, 1.0f);
	const float AlignmentScore = AlignmentPenalty * FMath::Max(0.0f, FocusTargetAlignmentScoreWeight);

	const float DistanceReference = FocusTargetSearchDistance > KINDA_SMALL_NUMBER
		? FocusTargetSearchDistance
		: 10000.0f;

	const float NormalizedDistance = FMath::Clamp(
		DistanceSquared / FMath::Square(DistanceReference),
		0.0f,
		1.0f
	);

	const float DistanceScore = NormalizedDistance * FMath::Max(0.0f, FocusTargetDistanceScoreWeight);

	return AlignmentScore + DistanceScore;
}

void UVoraxiaCameraComponent::DrawFocusTargetSelectionDebug(
	const FVector& StartLocation,
	const FVector& TargetLocation,
	const FColor& Color,
	const FString& Label
) const
{
	if (!bDrawFocusTargetSelectionDebug)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	DrawDebugLine(
		World,
		StartLocation,
		TargetLocation,
		Color,
		false,
		FocusTargetSelectionDebugDuration,
		0,
		1.5f
	);

	DrawDebugSphere(
		World,
		TargetLocation,
		18.0f,
		12,
		Color,
		false,
		FocusTargetSelectionDebugDuration,
		0,
		1.5f
	);

	DrawDebugString(
		World,
		TargetLocation + FVector(0.0f, 0.0f, 28.0f),
		Label,
		nullptr,
		Color,
		FocusTargetSelectionDebugDuration,
		true
	);
}

void UVoraxiaCameraComponent::UpdateFocus(const float DeltaTime)
{
	if (!bEnableFocusSystem)
	{
		FocusAlpha = 0.0f;
		ResetFocusTarget();
		return;
	}

	if (FocusBlendTime <= KINDA_SMALL_NUMBER)
	{
		FocusAlpha = FocusBlendTargetAlpha;
	}
	else
	{
		FocusBlendElapsed += DeltaTime;

		const float RawAlpha = FMath::Clamp(
			FocusBlendElapsed / FocusBlendTime,
			0.0f,
			1.0f
		);

		const float SmoothAlpha = RawAlpha * RawAlpha * (3.0f - 2.0f * RawAlpha);

		FocusAlpha = FMath::Lerp(
			FocusBlendStartAlpha,
			FocusBlendTargetAlpha,
			SmoothAlpha
		);

		if (RawAlpha >= 1.0f)
		{
			FocusAlpha = FocusBlendTargetAlpha;
			FocusBlendTime = 0.0f;
			FocusBlendElapsed = 0.0f;
		}
	}

	if (!IsFocusActive())
	{
		ResetFocusTarget();
		return;
	}

	FVector FocusLocation;

	if (!TryGetFocusLocation(FocusLocation))
	{
		ClearFocus(DefaultFocusBlendOutTime);
		return;
	}

	CachedFocusLocation = FocusLocation;
	bHasCachedFocusLocation = true;

	if (FocusAlpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector PivotLocation = CalculatePivotLocation();
	const FVector ToFocus = FocusLocation - PivotLocation;

	if (ToFocus.SizeSquared() < FMath::Square(FocusMinimumTargetDistance))
	{
		return;
	}

	FRotator TargetFocusRotation = ToFocus.Rotation();
	TargetFocusRotation.Roll = 0.0f;

	if (bClampFocusPitchToCameraLimits)
	{
		TargetFocusRotation.Pitch = FMath::Clamp(
			TargetFocusRotation.Pitch,
			MinPitch,
			MaxPitch
		);
	}

	const float YawDelta = FMath::FindDeltaAngleDegrees(
		DesiredRotation.Yaw,
		TargetFocusRotation.Yaw
	);

	DesiredRotation.Yaw += YawDelta * FocusAlpha;

	DesiredRotation.Pitch = FMath::Lerp(
		DesiredRotation.Pitch,
		TargetFocusRotation.Pitch,
		FocusAlpha
	);
	
	if (bDrawFocusDebug)
	{
		const UWorld* World = GetWorld();

		if (World)
		{
			DrawDebugSphere(
				World,
				FocusLocation,
				24.0f,
				16,
				FColor::Orange,
				false,
				0.0f,
				0,
				2.0f
			);

			DrawDebugLine(
				World,
				PivotLocation,
				FocusLocation,
				FColor::Orange,
				false,
				0.0f,
				0,
				1.5f
			);

			DrawDebugString(
				World,
				FocusLocation + FVector(0.0f, 0.0f, 40.0f),
				FString::Printf(
					TEXT("Focus %.2f | %s"),
					FocusAlpha,
					*GetCurrentFocusTargetName()
				),
				nullptr,
				FColor::Orange,
				0.0f,
				true
			);
		}
	}
}

void UVoraxiaCameraComponent::BeginFocusBlend(
	const float TargetAlpha,
	const float BlendTime
)
{
	FocusBlendStartAlpha = FocusAlpha;
	FocusBlendTargetAlpha = FMath::Clamp(TargetAlpha, 0.0f, 1.0f);
	FocusBlendTime = FMath::Max(0.0f, BlendTime);
	FocusBlendElapsed = 0.0f;

	if (FocusBlendTime <= KINDA_SMALL_NUMBER)
	{
		FocusAlpha = FocusBlendTargetAlpha;
	}
}

bool UVoraxiaCameraComponent::TryGetFocusLocation(FVector& OutFocusLocation) const
{
	if (bFocusActive)
	{
		if (const USceneComponent* Component = FocusTargetComponent.Get())
		{
			if (FocusTargetSocketName != NAME_None && Component->DoesSocketExist(FocusTargetSocketName))
			{
				OutFocusLocation = Component->GetSocketLocation(FocusTargetSocketName);
				return true;
			}

			OutFocusLocation = Component->GetComponentLocation();
			return true;
		}

		if (AActor* Actor = FocusTargetActor.Get())
		{
			if (const USceneComponent* RootComponent = Actor->GetRootComponent())
			{
				if (FocusTargetSocketName != NAME_None && RootComponent->DoesSocketExist(FocusTargetSocketName))
				{
					OutFocusLocation = RootComponent->GetSocketLocation(FocusTargetSocketName);
					return true;
				}
			}

			if (Actor->GetClass()->ImplementsInterface(UVoraxiaFocusableInterface::StaticClass()))
			{
				OutFocusLocation = IVoraxiaFocusableInterface::Execute_GetFocusLocation(
					Actor,
					GetOwner()
				);

				return true;
			}

			OutFocusLocation = Actor->GetActorLocation();
			return true;
		}

		if (bFocusUsesWorldLocation)
		{
			OutFocusLocation = FocusWorldLocation;
			return true;
		}
	}

	if (FocusAlpha > KINDA_SMALL_NUMBER && bHasCachedFocusLocation)
	{
		OutFocusLocation = CachedFocusLocation;
		return true;
	}

	return false;
}

void UVoraxiaCameraComponent::ResetFocusTarget()
{
	if (FocusAlpha > KINDA_SMALL_NUMBER)
	{
		return;
	}

	FocusTargetActor.Reset();
	FocusTargetComponent.Reset();

	FocusTargetSocketName = NAME_None;

	FocusWorldLocation = FVector::ZeroVector;
	CachedFocusLocation = FVector::ZeroVector;

	bFocusUsesWorldLocation = false;
	bFocusActive = false;
	bHasCachedFocusLocation = false;

	FocusBlendStartAlpha = 0.0f;
	FocusBlendTargetAlpha = 0.0f;
	FocusBlendTime = 0.0f;
	FocusBlendElapsed = 0.0f;
}

void UVoraxiaCameraComponent::UpdateDynamicModifiers(const float DeltaTime)
{
	const float TargetPitchDistanceOffset = CalculatePitchDistanceOffsetTarget();
	const float TargetPitchFOVOffset = CalculatePitchFOVOffsetTarget();

	const float TargetSpeedDistanceOffset = CalculateSpeedDistanceOffsetTarget();
	const float TargetSpeedFOVOffset = CalculateSpeedFOVOffsetTarget();

	const float SafePitchInterpSpeed = FMath::Max(0.0f, PitchModifierInterpSpeed);
	const float SafeSpeedInterpSpeed = FMath::Max(0.0f, SpeedModifierInterpSpeed);

	if (SafePitchInterpSpeed <= KINDA_SMALL_NUMBER)
	{
		CurrentPitchDistanceOffset = TargetPitchDistanceOffset;
		CurrentPitchFOVOffset = TargetPitchFOVOffset;
	}
	else
	{
		CurrentPitchDistanceOffset = FMath::FInterpTo(
			CurrentPitchDistanceOffset,
			TargetPitchDistanceOffset,
			DeltaTime,
			SafePitchInterpSpeed
		);

		CurrentPitchFOVOffset = FMath::FInterpTo(
			CurrentPitchFOVOffset,
			TargetPitchFOVOffset,
			DeltaTime,
			SafePitchInterpSpeed
		);
	}

	if (SafeSpeedInterpSpeed <= KINDA_SMALL_NUMBER)
	{
		CurrentSpeedDistanceOffset = TargetSpeedDistanceOffset;
		CurrentSpeedFOVOffset = TargetSpeedFOVOffset;
	}
	else
	{
		CurrentSpeedDistanceOffset = FMath::FInterpTo(
			CurrentSpeedDistanceOffset,
			TargetSpeedDistanceOffset,
			DeltaTime,
			SafeSpeedInterpSpeed
		);

		CurrentSpeedFOVOffset = FMath::FInterpTo(
			CurrentSpeedFOVOffset,
			TargetSpeedFOVOffset,
			DeltaTime,
			SafeSpeedInterpSpeed
		);
	}
}

float UVoraxiaCameraComponent::CalculatePitchDistanceOffsetTarget() const
{
	if (!bEnablePitchDistanceModifier)
	{
		return 0.0f;
	}

	const float SafeMinPitch = FMath::Min(MinPitch, MaxPitch);
	const float SafeMaxPitch = FMath::Max(MinPitch, MaxPitch);

	const float ClampedPitch = FMath::Clamp(SmoothedRotation.Pitch, SafeMinPitch, SafeMaxPitch);
	const float ClampedRestingPitch = FMath::Clamp(RestingCameraPitch, SafeMinPitch, SafeMaxPitch);

	if (ClampedPitch < ClampedRestingPitch)
	{
		const float Range = FMath::Max(KINDA_SMALL_NUMBER, ClampedRestingPitch - SafeMinPitch);
		const float Alpha = FMath::Clamp((ClampedRestingPitch - ClampedPitch) / Range, 0.0f, 1.0f);
		const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);

		return PitchDistanceAtMinPitchOffset * SmoothAlpha;
	}

	const float Range = FMath::Max(KINDA_SMALL_NUMBER, SafeMaxPitch - ClampedRestingPitch);
	const float Alpha = FMath::Clamp((ClampedPitch - ClampedRestingPitch) / Range, 0.0f, 1.0f);
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);

	return PitchDistanceAtMaxPitchOffset * SmoothAlpha;
}

float UVoraxiaCameraComponent::CalculatePitchFOVOffsetTarget() const
{
	if (!bEnablePitchFOVModifier)
	{
		return 0.0f;
	}

	const float SafeMinPitch = FMath::Min(MinPitch, MaxPitch);
	const float SafeMaxPitch = FMath::Max(MinPitch, MaxPitch);

	const float ClampedPitch = FMath::Clamp(SmoothedRotation.Pitch, SafeMinPitch, SafeMaxPitch);
	const float ClampedRestingPitch = FMath::Clamp(RestingCameraPitch, SafeMinPitch, SafeMaxPitch);

	if (ClampedPitch < ClampedRestingPitch)
	{
		const float Range = FMath::Max(KINDA_SMALL_NUMBER, ClampedRestingPitch - SafeMinPitch);
		const float Alpha = FMath::Clamp((ClampedRestingPitch - ClampedPitch) / Range, 0.0f, 1.0f);
		const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);

		return PitchFOVAtMinPitchOffset * SmoothAlpha;
	}

	const float Range = FMath::Max(KINDA_SMALL_NUMBER, SafeMaxPitch - ClampedRestingPitch);
	const float Alpha = FMath::Clamp((ClampedPitch - ClampedRestingPitch) / Range, 0.0f, 1.0f);
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);

	return PitchFOVAtMaxPitchOffset * SmoothAlpha;
}

float UVoraxiaCameraComponent::CalculateSpeedDistanceOffsetTarget() const
{
	if (!bEnableSpeedDistanceModifier)
	{
		return 0.0f;
	}

	return SpeedDistanceOffset * CalculateSpeedModifierAlpha();
}

float UVoraxiaCameraComponent::CalculateSpeedFOVOffsetTarget() const
{
	if (!bEnableSpeedFOVModifier)
	{
		return 0.0f;
	}

	return SpeedFOVOffset * CalculateSpeedModifierAlpha();
}

float UVoraxiaCameraComponent::CalculateSpeedModifierAlpha() const
{
	const AActor* Owner = GetOwner();

	if (!Owner || SpeedModifierFullSpeed <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float Speed = Owner->GetVelocity().Size2D();
	const float Alpha = FMath::Clamp(Speed / SpeedModifierFullSpeed, 0.0f, 1.0f);

	return Alpha * Alpha * (3.0f - 2.0f * Alpha);
}

void UVoraxiaCameraComponent::UpdateMovementAnticipation(const float DeltaTime)
{
	const FVector TargetOffset = CalculateMovementAnticipationTarget();

	const bool bReturningToNeutral = TargetOffset.IsNearlyZero();

	const float InterpSpeed = bReturningToNeutral
		? MovementAnticipationReturnSpeed
		: MovementAnticipationInterpSpeed;

	if (!bEnableMovementAnticipation)
	{
		CurrentMovementAnticipationOffset = FVector::ZeroVector;
		return;
	}

	if (InterpSpeed <= KINDA_SMALL_NUMBER || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		CurrentMovementAnticipationOffset = TargetOffset;
		return;
	}

	CurrentMovementAnticipationOffset = FMath::VInterpTo(
		CurrentMovementAnticipationOffset,
		TargetOffset,
		DeltaTime,
		InterpSpeed
	);
}

FVector UVoraxiaCameraComponent::CalculateMovementAnticipationTarget() const
{
	if (!bEnableMovementAnticipation)
	{
		return FVector::ZeroVector;
	}

	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	const FVector Velocity = Owner->GetVelocity();
	const FVector FlatVelocity(Velocity.X, Velocity.Y, 0.0f);

	const float Speed = FlatVelocity.Size();

	if (Speed <= KINDA_SMALL_NUMBER || MovementAnticipationFullSpeed <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	const float SpeedAlpha = FMath::Clamp(
		Speed / MovementAnticipationFullSpeed,
		0.0f,
		1.0f
	);

	if (SpeedAlpha < MovementAnticipationDeadZone)
	{
		return FVector::ZeroVector;
	}

	const FVector MovementDirection = FlatVelocity / Speed;

	const FRotator CameraYawRotation(0.0f, SmoothedRotation.Yaw, 0.0f);
	const FRotationMatrix CameraYawMatrix(CameraYawRotation);

	const FVector CameraForward = CameraYawMatrix.GetUnitAxis(EAxis::X).GetSafeNormal2D();
	const FVector CameraRight = CameraYawMatrix.GetUnitAxis(EAxis::Y).GetSafeNormal2D();

	const float ForwardAmount = FVector::DotProduct(MovementDirection, CameraForward);
	const float RightAmount = FVector::DotProduct(MovementDirection, CameraRight);

	const float ForwardOffset = ForwardAmount >= 0.0f
		? ForwardAmount * MovementAnticipationForwardOffset
		: ForwardAmount * MovementAnticipationBackwardOffset;

	const float SideOffset = RightAmount * MovementAnticipationSideOffset;

	float LookMultiplier = 1.0f;

	if (bReduceMovementAnticipationWhileLooking)
	{
		const bool bRecentlyLooked =
			TimeSinceLastYawInput < MovementAnticipationLookInputThreshold
			|| TimeSinceLastPitchInput < MovementAnticipationLookInputThreshold;

		if (bRecentlyLooked)
		{
			LookMultiplier = FMath::Clamp(
				LookInputAnticipationMultiplier,
				0.0f,
				1.0f
			);
		}
	}

	return FVector(
		ForwardOffset,
		SideOffset,
		0.0f
	) * SpeedAlpha * LookMultiplier;
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

	const float Distance = FMath::Max(
		0.0f,
		RuntimeCameraDistance.CurrentValue + GetCurrentDynamicDistanceOffset()
	);
	
	const FVector EffectiveCameraOffset =
		GetCurrentCameraOffset()
		+ CurrentMovementAnticipationOffset;

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
	if (!IsLocalPresentationOwner())
	{
		return;
	}

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
		BaseFOV
		+ RuntimeFOVOffset.CurrentValue
		+ GetCurrentDynamicFOVOffset(),
		MinFOV,
		MaxFOV
	);
}