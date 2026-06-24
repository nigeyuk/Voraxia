// Copyright 2026 Coding Custard Studios.

#include "VoraxiaBlueprintPickupActor.h"
#include "VoraxiaBlueprintDataAsset.h"
#include "Player/VoraxiaPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"

AVoraxiaBlueprintPickupActor::AVoraxiaBlueprintPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
}

void AVoraxiaBlueprintPickupActor::Interact_Implementation(AActor* InteractingActor)
{
	AVoraxiaPlayerCharacter* PlayerCharacter = Cast<AVoraxiaPlayerCharacter>(InteractingActor);
	if (!PlayerCharacter || !BlueprintData)
	{
		return;
	}

	if (PlayerCharacter->AddPhysicalBlueprint(BlueprintData))
	{
		Destroy();
	}
}

FText AVoraxiaBlueprintPickupActor::GetInteractionText_Implementation() const
{
	if (BlueprintData)
	{
		return FText::Format(
			NSLOCTEXT("Voraxia", "PickupBlueprintFormat", "Pick up {0}"),
			BlueprintData->DisplayName
		);
	}

	return NSLOCTEXT("Voraxia", "PickupBlueprintFallback", "Pick up blueprint");
}
