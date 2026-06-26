// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoraxiaInteractableInterface.h"
#include "VoraxiaBlueprintStorageChest.generated.h"

class UStaticMeshComponent;
class UVoraxiaBlueprintDataAsset;

UCLASS()
class VORAXIA_API AVoraxiaBlueprintStorageChest : public AActor, public IVoraxiaInteractableInterface
{
	GENERATED_BODY()

public:
	AVoraxiaBlueprintStorageChest();

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() const override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

protected:
	virtual void BeginPlay() override;

	void UpdateChestColour();

	UFUNCTION()
	void OnRep_StoredBlueprint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(
		ReplicatedUsing = OnRep_StoredBlueprint,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Storage"
	)
	TObjectPtr<UVoraxiaBlueprintDataAsset> StoredBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage")
	FLinearColor EmptyColour = FLinearColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage")
	FLinearColor FilledColour = FLinearColor::Green;
};
