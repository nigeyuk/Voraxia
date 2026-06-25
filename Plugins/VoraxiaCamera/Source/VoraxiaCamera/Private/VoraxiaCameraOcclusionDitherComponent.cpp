// Copyright 2026 Coding Custard Studios.

#include "VoraxiaCameraOcclusionDitherComponent.h"

#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "VoraxiaCameraComponent.h"
#include "VoraxiaCameraSettingsAsset.h"
#include "VoraxiaCameraLog.h"

UVoraxiaCameraOcclusionDitherComponent::UVoraxiaCameraOcclusionDitherComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UVoraxiaCameraOcclusionDitherComponent::BeginPlay()
{
	Super::BeginPlay();

	/*
	 * As with the camera component, wait for local possession if this Pawn
	 * begins play before its owning client is fully established.
	 */
	if (IsLocalPresentationOwner())
	{
		InitializeLocalPresentation();
	}
}

void UVoraxiaCameraOcclusionDitherComponent::InitializeLocalPresentation()
{
	if (bLocalPresentationInitialized || !IsLocalPresentationOwner())
	{
		return;
	}

	if (!CameraComponent.IsValid() && bAutoFindCameraComponent)
	{
		if (AActor* Owner = GetOwner())
		{
			CameraComponent = Owner->FindComponentByClass<UVoraxiaCameraComponent>();
		}
	}

	if (UVoraxiaCameraComponent* ResolvedCameraComponent = CameraComponent.Get())
	{
		AddTickPrerequisiteComponent(ResolvedCameraComponent);
	}
	else
	{
		UE_LOG(
			LogVoraxiaCamera,
			Warning,
			TEXT("Voraxia occlusion dither component could not find UVoraxiaCameraComponent on owner '%s'."),
			*GetNameSafe(GetOwner())
		);
	}

	bLocalPresentationInitialized = true;
}

void UVoraxiaCameraOcclusionDitherComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreAllOriginalMaterials();

	Super::EndPlay(EndPlayReason);
}

void UVoraxiaCameraOcclusionDitherComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLocalPresentationOwner())
	{
		if (bLocalPresentationInitialized)
		{
			RestoreAllOriginalMaterials();
			bLocalPresentationInitialized = false;
		}

		return;
	}

	if (!bLocalPresentationInitialized)
	{
		InitializeLocalPresentation();
	}

	if (!bLocalPresentationInitialized)
	{
		return;
	}

	if (!CameraComponent.IsValid() && bAutoFindCameraComponent)
	{
		if (AActor* Owner = GetOwner())
		{
			SetCameraComponent(Owner->FindComponentByClass<UVoraxiaCameraComponent>());
		}
	}

	if (bEnableCameraOcclusionDither && CameraComponent.IsValid())
	{
		TraceTimeAccumulator += FMath::Max(0.0f, DeltaTime);

		if (DitherTraceInterval <= KINDA_SMALL_NUMBER || TraceTimeAccumulator >= DitherTraceInterval)
		{
			TraceTimeAccumulator = 0.0f;
			RefreshOccludingComponents();
		}
	}
	else
	{
		ActiveOccludingComponentCount = 0;

		for (FTrackedDitherComponent& TrackedComponent : TrackedComponents)
		{
			TrackedComponent.bShouldDither = false;
		}
	}

	UpdateTrackedComponentFades(DeltaTime);
}

bool UVoraxiaCameraOcclusionDitherComponent::IsLocalPresentationOwner() const
{
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	return OwningPawn && OwningPawn->IsLocallyControlled();
}

void UVoraxiaCameraOcclusionDitherComponent::SetCameraComponent(
	UVoraxiaCameraComponent* InCameraComponent
)
{
	if (CameraComponent.Get() == InCameraComponent)
	{
		return;
	}

	if (UVoraxiaCameraComponent* PreviousCameraComponent = CameraComponent.Get())
	{
		RemoveTickPrerequisiteComponent(PreviousCameraComponent);
	}

	CameraComponent = InCameraComponent;

	if (UVoraxiaCameraComponent* NewCameraComponent = CameraComponent.Get())
	{
		AddTickPrerequisiteComponent(NewCameraComponent);
	}
}

UVoraxiaCameraComponent* UVoraxiaCameraOcclusionDitherComponent::GetCameraComponent() const
{
	return CameraComponent.Get();
}

int32 UVoraxiaCameraOcclusionDitherComponent::GetTrackedDitheredComponentCount() const
{
	return TrackedComponents.Num();
}

int32 UVoraxiaCameraOcclusionDitherComponent::GetActiveOccludingComponentCount() const
{
	return ActiveOccludingComponentCount;
}


void UVoraxiaCameraOcclusionDitherComponent::CaptureSettings(
	FVoraxiaCameraOcclusionDitherSettings& OutSettings
) const
{
	OutSettings.bEnableCameraOcclusionDither = bEnableCameraOcclusionDither;
	OutSettings.bAutoFindCameraComponent = bAutoFindCameraComponent;
	OutSettings.DitherTraceChannel = DitherTraceChannel;
	OutSettings.DitherProbeRadius = DitherProbeRadius;
	OutSettings.DitherTraceInterval = DitherTraceInterval;
	OutSettings.MaxDitheredComponents = MaxDitheredComponents;
	OutSettings.DitherScalarParameterName = DitherScalarParameterName;
	OutSettings.OccludedDitherFadeValue = OccludedDitherFadeValue;
	OutSettings.FadeOutInterpSpeed = FadeOutInterpSpeed;
	OutSettings.FadeInInterpSpeed = FadeInInterpSpeed;
	OutSettings.RestoredStateReleaseDelay = RestoredStateReleaseDelay;
	OutSettings.bDrawDitherDebug = bDrawDitherDebug;
	OutSettings.bLogDitherChanges = bLogDitherChanges;
}

void UVoraxiaCameraOcclusionDitherComponent::ApplySettings(
	const FVoraxiaCameraOcclusionDitherSettings& InSettings
)
{
	// Existing cached MIDs may point at an old parameter name or material setup.
	// Restore the source materials before swapping a preset.
	RestoreAllOriginalMaterials();

	bEnableCameraOcclusionDither = InSettings.bEnableCameraOcclusionDither;
	bAutoFindCameraComponent = InSettings.bAutoFindCameraComponent;
	DitherTraceChannel = InSettings.DitherTraceChannel;
	DitherProbeRadius = InSettings.DitherProbeRadius;
	DitherTraceInterval = InSettings.DitherTraceInterval;
	MaxDitheredComponents = InSettings.MaxDitheredComponents;
	DitherScalarParameterName = InSettings.DitherScalarParameterName;
	OccludedDitherFadeValue = InSettings.OccludedDitherFadeValue;
	FadeOutInterpSpeed = InSettings.FadeOutInterpSpeed;
	FadeInInterpSpeed = InSettings.FadeInInterpSpeed;
	RestoredStateReleaseDelay = InSettings.RestoredStateReleaseDelay;
	bDrawDitherDebug = InSettings.bDrawDitherDebug;
	bLogDitherChanges = InSettings.bLogDitherChanges;
	TraceTimeAccumulator = 0.0f;
}

void UVoraxiaCameraOcclusionDitherComponent::RefreshOccludingComponents()
{
	UVoraxiaCameraComponent* ResolvedCameraComponent = CameraComponent.Get();
	UWorld* World = GetWorld();

	if (!ResolvedCameraComponent || !World)
	{
		return;
	}

	for (FTrackedDitherComponent& TrackedComponent : TrackedComponents)
	{
		TrackedComponent.bShouldDither = false;
	}

	const FVector PivotLocation = ResolvedCameraComponent->GetLastPivotLocation();
	const FVector FinalCameraLocation = ResolvedCameraComponent->GetLastFinalCameraLocation();

	if (PivotLocation.Equals(FinalCameraLocation, 0.1f))
	{
		ActiveOccludingComponentCount = 0;
		return;
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(VoraxiaCameraOcclusionDither),
		false
	);

	QueryParams.AddIgnoredActor(GetOwner());

	TArray<FHitResult> Hits;

	World->SweepMultiByChannel(
		Hits,
		PivotLocation,
		FinalCameraLocation,
		FQuat::Identity,
		DitherTraceChannel,
		FCollisionShape::MakeSphere(FMath::Max(0.0f, DitherProbeRadius)),
		QueryParams
	);

	if (bDrawDitherDebug)
	{
		DrawDitherTraceDebug(PivotLocation, FinalCameraLocation, Hits);
	}

	TSet<UPrimitiveComponent*> UniqueComponents;
	ActiveOccludingComponentCount = 0;

	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* HitComponent = Hit.GetComponent();

		if (!HitComponent || HitComponent->GetOwner() == GetOwner())
		{
			continue;
		}

		if (!HitComponent->IsVisible() || HitComponent->GetNumMaterials() <= 0)
		{
			continue;
		}

		if (UniqueComponents.Contains(HitComponent))
		{
			continue;
		}

		if (ActiveOccludingComponentCount >= MaxDitheredComponents)
		{
			break;
		}

		UniqueComponents.Add(HitComponent);

		if (!EnsureTrackedDitherComponent(HitComponent))
		{
			continue;
		}

		FTrackedDitherComponent* TrackedComponent = FindTrackedComponent(HitComponent);

		if (!TrackedComponent)
		{
			continue;
		}

		TrackedComponent->bShouldDither = true;
		TrackedComponent->TimeFullyRestored = 0.0f;
		++ActiveOccludingComponentCount;
	}
}

void UVoraxiaCameraOcclusionDitherComponent::UpdateTrackedComponentFades(const float DeltaTime)
{
	for (int32 Index = TrackedComponents.Num() - 1; Index >= 0; --Index)
	{
		FTrackedDitherComponent& TrackedComponent = TrackedComponents[Index];
		UPrimitiveComponent* Component = TrackedComponent.Component.Get();

		if (!Component)
		{
			TrackedComponents.RemoveAtSwap(Index);
			continue;
		}

		const float TargetFade = TrackedComponent.bShouldDither
			? FMath::Clamp(OccludedDitherFadeValue, 0.0f, 1.0f)
			: 1.0f;

		const float InterpSpeed = TrackedComponent.bShouldDither
			? FMath::Max(0.0f, FadeOutInterpSpeed)
			: FMath::Max(0.0f, FadeInInterpSpeed);

		if (InterpSpeed <= KINDA_SMALL_NUMBER || DeltaTime <= KINDA_SMALL_NUMBER)
		{
			TrackedComponent.CurrentFade = TargetFade;
		}
		else
		{
			TrackedComponent.CurrentFade = FMath::FInterpTo(
				TrackedComponent.CurrentFade,
				TargetFade,
				DeltaTime,
				InterpSpeed
			);
		}

		ApplyFadeToTrackedComponent(TrackedComponent, TrackedComponent.CurrentFade);

		if (bLogDitherChanges && TrackedComponent.bShouldDither != TrackedComponent.bWasDitheringLastUpdate)
		{
			UE_LOG(
				LogVoraxiaCamera,
				Log,
				TEXT("Voraxia camera occlusion dither %s '%s'."),
				TrackedComponent.bShouldDither ? TEXT("started on") : TEXT("cleared from"),
				*GetNameSafe(Component)
			);

			TrackedComponent.bWasDitheringLastUpdate = TrackedComponent.bShouldDither;
		}

		if (TrackedComponent.bShouldDither)
		{
			TrackedComponent.TimeFullyRestored = 0.0f;
			continue;
		}

		if (!FMath::IsNearlyEqual(TrackedComponent.CurrentFade, 1.0f, 0.001f))
		{
			TrackedComponent.TimeFullyRestored = 0.0f;
			continue;
		}

		TrackedComponent.TimeFullyRestored += FMath::Max(0.0f, DeltaTime);

		if (TrackedComponent.TimeFullyRestored >= RestoredStateReleaseDelay)
		{
			RestoreOriginalMaterials(TrackedComponent);
			TrackedComponents.RemoveAtSwap(Index);
		}
	}
}

bool UVoraxiaCameraOcclusionDitherComponent::EnsureTrackedDitherComponent(
	UPrimitiveComponent* Component
)
{
	if (!Component)
	{
		return false;
	}

	if (FindTrackedComponent(Component))
	{
		return true;
	}

	FTrackedDitherComponent NewTrackedComponent;
	NewTrackedComponent.Component = Component;

	for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
	{
		UMaterialInterface* OriginalMaterial = Component->GetMaterial(MaterialIndex);

		if (!DoesMaterialSupportDitherParameter(OriginalMaterial))
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = Component->CreateDynamicMaterialInstance(
			MaterialIndex,
			OriginalMaterial
		);

		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetScalarParameterValue(DitherScalarParameterName, 1.0f);

		FMaterialDitherSlot& MaterialSlot = NewTrackedComponent.MaterialSlots.AddDefaulted_GetRef();
		MaterialSlot.MaterialIndex = MaterialIndex;
		MaterialSlot.OriginalMaterial = OriginalMaterial;
		MaterialSlot.DynamicMaterial = DynamicMaterial;
	}

	if (NewTrackedComponent.MaterialSlots.IsEmpty())
	{
		return false;
	}

	TrackedComponents.Add(MoveTemp(NewTrackedComponent));
	return true;
}

bool UVoraxiaCameraOcclusionDitherComponent::DoesMaterialSupportDitherParameter(
	const UMaterialInterface* Material
) const
{
	if (!Material || DitherScalarParameterName.IsNone())
	{
		return false;
	}

	TArray<FMaterialParameterInfo> ScalarParameterInfo;
	TArray<FGuid> ScalarParameterIds;

	Material->GetAllScalarParameterInfo(
		ScalarParameterInfo,
		ScalarParameterIds
	);

	return ScalarParameterInfo.ContainsByPredicate(
		[this](const FMaterialParameterInfo& ParameterInfo)
		{
			return ParameterInfo.Name == DitherScalarParameterName;
		}
	);
}

UVoraxiaCameraOcclusionDitherComponent::FTrackedDitherComponent*
UVoraxiaCameraOcclusionDitherComponent::FindTrackedComponent(
	UPrimitiveComponent* Component
)
{
	return TrackedComponents.FindByPredicate(
		[Component](const FTrackedDitherComponent& TrackedComponent)
		{
			return TrackedComponent.Component.Get() == Component;
		}
	);
}

const UVoraxiaCameraOcclusionDitherComponent::FTrackedDitherComponent*
UVoraxiaCameraOcclusionDitherComponent::FindTrackedComponent(
	const UPrimitiveComponent* Component
) const
{
	return TrackedComponents.FindByPredicate(
		[Component](const FTrackedDitherComponent& TrackedComponent)
		{
			return TrackedComponent.Component.Get() == Component;
		}
	);
}

void UVoraxiaCameraOcclusionDitherComponent::ApplyFadeToTrackedComponent(
	FTrackedDitherComponent& TrackedComponent,
	const float FadeValue
) const
{
	for (FMaterialDitherSlot& MaterialSlot : TrackedComponent.MaterialSlots)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = MaterialSlot.DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(
				DitherScalarParameterName,
				FadeValue
			);
		}
	}
}

void UVoraxiaCameraOcclusionDitherComponent::RestoreOriginalMaterials(
	FTrackedDitherComponent& TrackedComponent
) const
{
	UPrimitiveComponent* Component = TrackedComponent.Component.Get();

	if (!Component)
	{
		return;
	}

	for (const FMaterialDitherSlot& MaterialSlot : TrackedComponent.MaterialSlots)
	{
		if (MaterialSlot.MaterialIndex != INDEX_NONE && MaterialSlot.OriginalMaterial)
		{
			Component->SetMaterial(
				MaterialSlot.MaterialIndex,
				MaterialSlot.OriginalMaterial
			);
		}
	}
}

void UVoraxiaCameraOcclusionDitherComponent::RestoreAllOriginalMaterials()
{
	for (FTrackedDitherComponent& TrackedComponent : TrackedComponents)
	{
		RestoreOriginalMaterials(TrackedComponent);
	}

	TrackedComponents.Reset();
	ActiveOccludingComponentCount = 0;
}

void UVoraxiaCameraOcclusionDitherComponent::DrawDitherTraceDebug(
	const FVector& PivotLocation,
	const FVector& FinalCameraLocation,
	const TArray<FHitResult>& Hits
) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	DrawDebugLine(
		World,
		PivotLocation,
		FinalCameraLocation,
		FColor::Cyan,
		false,
		0.0f,
		0,
		1.5f
	);

	DrawDebugSphere(
		World,
		PivotLocation,
		FMath::Max(4.0f, DitherProbeRadius),
		12,
		FColor::Cyan,
		false,
		0.0f,
		0,
		1.0f
	);

	for (const FHitResult& Hit : Hits)
	{
		DrawDebugSphere(
			World,
			Hit.ImpactPoint,
			12.0f,
			12,
			FColor::Purple,
			false,
			0.0f,
			0,
			1.0f
		);
	}

	for (const FTrackedDitherComponent& TrackedComponent : TrackedComponents)
	{
		if (!TrackedComponent.bShouldDither)
		{
			continue;
		}

		if (const UPrimitiveComponent* Component = TrackedComponent.Component.Get())
		{
			DrawDebugBox(
				World,
				Component->Bounds.Origin,
				Component->Bounds.BoxExtent,
				FColor::Green,
				false,
				0.0f,
				0,
				1.0f
			);
		}
	}
}
