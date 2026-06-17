#include "Mining/VoraxiaDissolveAsteroidActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "VoraxiaLog.h"

AVoraxiaDissolveAsteroidActor::AVoraxiaDissolveAsteroidActor()
{
	PrimaryActorTick.bCanEverTick = false;

	AsteroidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AsteroidMesh"));
	SetRootComponent(AsteroidMesh);

	AsteroidMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AsteroidMesh->SetCollisionObjectType(ECC_WorldStatic);
	AsteroidMesh->SetCollisionResponseToAllChannels(ECR_Block);

	MaxWounds = 4;
}

void AVoraxiaDissolveAsteroidActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureWoundSlots();
	CreateDynamicMaterial();
	UpdateDissolveMaterialParameters();

	UE_LOG(LogVoraxia, Log, TEXT("Dissolve asteroid ready: %s"), *GetName());
}

void AVoraxiaDissolveAsteroidActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EnsureWoundSlots();
	CreateDynamicMaterial();
	UpdateDissolveMaterialParameters();
}

void AVoraxiaDissolveAsteroidActor::ApplyRaptorMining_Implementation(
	const FHitResult& Hit,
	float MiningPower,
	float DeltaSeconds
)
{
	if (!AsteroidMesh)
	{
		return;
	}

	EnsureWoundSlots();
	CreateDynamicMaterial();

	if (!DynamicMaterial)
	{
		return;
	}

	const FVector LocalHitPosition = AsteroidMesh->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
	const int32 WoundIndex = FindBestWoundSlot(LocalHitPosition);

	if (!Wounds.IsValidIndex(WoundIndex))
	{
		return;
	}

	const float SafeMiningPower = FMath::Max(0.0f, MiningPower);
	const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);

	FVoraxiaDissolveWound& Wound = Wounds[WoundIndex];

	if (!Wound.bActive)
	{
		Wound.bActive = true;
		Wound.LocalPosition = LocalHitPosition;
		Wound.Radius = InitialWoundRadius;
		Wound.Strength = 0.0f;
	}
	else
	{
		// Let nearby hits gently pull the wound centre around instead of creating jittery speckles.
		Wound.LocalPosition = FMath::VInterpTo(
			Wound.LocalPosition,
			LocalHitPosition,
			SafeDeltaSeconds,
			8.0f
		);
	}

	Wound.Radius += RadiusGrowthPerSecond * SafeMiningPower * SafeDeltaSeconds;
	Wound.Strength += StrengthGrowthPerSecond * SafeMiningPower * SafeDeltaSeconds;
	Wound.Strength = FMath::Clamp(Wound.Strength, 0.0f, 1.0f);

	UpdateDissolveMaterialParameters();

	if (bDisableCollisionWhenFullyMined && GetTotalDissolveStrength() >= static_cast<float>(MaxWounds))
	{
		AsteroidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AVoraxiaDissolveAsteroidActor::CreateDynamicMaterial()
{
	if (!AsteroidMesh || DynamicMaterial)
	{
		return;
	}

	UMaterialInterface* SourceMaterial = AsteroidMesh->GetMaterial(0);

	if (!SourceMaterial)
	{
		return;
	}

	DynamicMaterial = AsteroidMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, SourceMaterial);
}

void AVoraxiaDissolveAsteroidActor::EnsureWoundSlots()
{
	MaxWounds = FMath::Clamp(MaxWounds, 1, 4);

	if (Wounds.Num() == MaxWounds)
	{
		return;
	}

	Wounds.SetNum(MaxWounds);
}

int32 AVoraxiaDissolveAsteroidActor::FindBestWoundSlot(const FVector& LocalPosition) const
{
	int32 FirstInactiveIndex = INDEX_NONE;

	for (int32 Index = 0; Index < Wounds.Num(); ++Index)
	{
		const FVoraxiaDissolveWound& Wound = Wounds[Index];

		if (!Wound.bActive)
		{
			if (FirstInactiveIndex == INDEX_NONE)
			{
				FirstInactiveIndex = Index;
			}

			continue;
		}

		const float Distance = FVector::Dist(Wound.LocalPosition, LocalPosition);

		if (Distance <= WoundMergeDistance)
		{
			return Index;
		}
	}

	if (FirstInactiveIndex != INDEX_NONE)
	{
		return FirstInactiveIndex;
	}

	// All wound slots are active, so reuse the weakest one.
	int32 WeakestIndex = 0;
	float WeakestStrength = TNumericLimits<float>::Max();

	for (int32 Index = 0; Index < Wounds.Num(); ++Index)
	{
		if (Wounds[Index].Strength < WeakestStrength)
		{
			WeakestStrength = Wounds[Index].Strength;
			WeakestIndex = Index;
		}
	}

	return WeakestIndex;
}

void AVoraxiaDissolveAsteroidActor::UpdateDissolveMaterialParameters()
{
	if (!DynamicMaterial)
	{
		return;
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FString PointName = FString::Printf(TEXT("DissolvePoint%d"), Index);
		const FString RadiusName = FString::Printf(TEXT("DissolveRadius%d"), Index);
		const FString StrengthName = FString::Printf(TEXT("DissolveStrength%d"), Index);

		if (Wounds.IsValidIndex(Index) && Wounds[Index].bActive)
		{
			const FVoraxiaDissolveWound& Wound = Wounds[Index];

			DynamicMaterial->SetVectorParameterValue(
				FName(*PointName),
				FLinearColor(
					Wound.LocalPosition.X,
					Wound.LocalPosition.Y,
					Wound.LocalPosition.Z,
					1.0f
				)
			);

			DynamicMaterial->SetScalarParameterValue(FName(*RadiusName), Wound.Radius);
			DynamicMaterial->SetScalarParameterValue(FName(*StrengthName), Wound.Strength);
		}
		else
		{
			DynamicMaterial->SetVectorParameterValue(FName(*PointName), FLinearColor::Black);
			DynamicMaterial->SetScalarParameterValue(FName(*RadiusName), 0.0f);
			DynamicMaterial->SetScalarParameterValue(FName(*StrengthName), 0.0f);
		}
	}
}

float AVoraxiaDissolveAsteroidActor::GetTotalDissolveStrength() const
{
	float Total = 0.0f;

	for (const FVoraxiaDissolveWound& Wound : Wounds)
	{
		if (Wound.bActive)
		{
			Total += Wound.Strength;
		}
	}

	return Total;
}