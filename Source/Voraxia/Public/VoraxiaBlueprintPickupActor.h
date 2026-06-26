// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoraxiaInteractableInterface.h"
#include "VoraxiaBlueprintPickupActor.generated.h"

class UStaticMeshComponent;
class UVoraxiaBlueprintDataAsset;

UCLASS()
class VORAXIA_API AVoraxiaBlueprintPickupActor : public AActor, public IVoraxiaInteractableInterface
{
	GENERATED_BODY()

public:
	AVoraxiaBlueprintPickupActor();

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() const override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Replicated,
		Category = "Blueprint"
	)
	TObjectPtr<UVoraxiaBlueprintDataAsset> BlueprintData;
};
