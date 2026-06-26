// Copyright 2026 Coding Custard Studios.

#include "VoraxiaBlueprintStorageChest.h"
#include "VoraxiaBlueprintDataAsset.h"
#include "Player/VoraxiaPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

AVoraxiaBlueprintStorageChest::AVoraxiaBlueprintStorageChest()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AVoraxiaBlueprintStorageChest::BeginPlay()
{
	Super::BeginPlay();

	UpdateChestColour();
}

void AVoraxiaBlueprintStorageChest::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AVoraxiaBlueprintStorageChest, StoredBlueprint);
}

void AVoraxiaBlueprintStorageChest::Interact_Implementation(AActor* InteractingActor)
{
	/* Chest contents are shared world state and therefore server-authoritative. */
	if (!HasAuthority())
	{
		return;
	}

	AVoraxiaPlayerCharacter* PlayerCharacter =
		Cast<AVoraxiaPlayerCharacter>(InteractingActor);

	if (!PlayerCharacter)
	{
		return;
	}

	if (!StoredBlueprint)
	{
		UVoraxiaBlueprintDataAsset* PlayerBlueprint =
			PlayerCharacter->GetFirstPhysicalBlueprint();

		if (!PlayerBlueprint)
		{
			return;
		}

		if (PlayerCharacter->RemovePhysicalBlueprint(PlayerBlueprint))
		{
			StoredBlueprint = PlayerBlueprint;
			UpdateChestColour();
			ForceNetUpdate();
		}

		return;
	}

	if (PlayerCharacter->AddPhysicalBlueprint(StoredBlueprint))
	{
		StoredBlueprint = nullptr;
		UpdateChestColour();
		ForceNetUpdate();
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

void AVoraxiaBlueprintStorageChest::OnRep_StoredBlueprint()
{
	UpdateChestColour();
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

