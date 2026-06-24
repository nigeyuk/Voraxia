// Copyright 2026 Coding Custard Studios.

#include "VoraxiaBlueprintStorageChest.h"
#include "VoraxiaBlueprintDataAsset.h"
#include "Player/VoraxiaPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AVoraxiaBlueprintStorageChest::AVoraxiaBlueprintStorageChest()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
}

void AVoraxiaBlueprintStorageChest::BeginPlay()
{
	Super::BeginPlay();

	UpdateChestColour();
}

void AVoraxiaBlueprintStorageChest::Interact_Implementation(AActor* InteractingActor)
{
	AVoraxiaPlayerCharacter* PlayerCharacter = Cast<AVoraxiaPlayerCharacter>(InteractingActor);
	if (!PlayerCharacter)
	{
		return;
	}

	if (!StoredBlueprint)
	{
		UVoraxiaBlueprintDataAsset* PlayerBlueprint = PlayerCharacter->GetFirstPhysicalBlueprint();
		if (!PlayerBlueprint)
		{
			return;
		}

		if (PlayerCharacter->RemovePhysicalBlueprint(PlayerBlueprint))
		{
			StoredBlueprint = PlayerBlueprint;
			UpdateChestColour();
		}

		return;
	}

	if (PlayerCharacter->AddPhysicalBlueprint(StoredBlueprint))
	{
		StoredBlueprint = nullptr;
		UpdateChestColour();
	}
}

FText AVoraxiaBlueprintStorageChest::GetInteractionText_Implementation() const
{
	if (StoredBlueprint)
	{
		return NSLOCTEXT("Voraxia", "WithdrawBlueprint", "Take blueprint");
	}

	return NSLOCTEXT("Voraxia", "DepositBlueprint", "Store blueprint");
}

void AVoraxiaBlueprintStorageChest::UpdateChestColour()
{
	UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	if (!DynamicMaterial)
	{
		return;
	}

	const FLinearColor TargetColour = StoredBlueprint ? FilledColour : EmptyColour;

	DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TargetColour);
	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TargetColour);
}

