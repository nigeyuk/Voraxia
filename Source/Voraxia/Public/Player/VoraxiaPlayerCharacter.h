// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Mining/VoraxiaMiningInventoryReceiver.h"
#include "VoraxiaPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UVoraxiaCameraComponent;
class UVoraxiaCameraOcclusionDitherComponent;
class UVoraxiaRaptorMiningComponent;
class UVoraxiaResourceInventoryComponent;
class SVoraxiaMiningLedgerWidget;
class UVoraxiaBlueprintDataAsset;

struct FInputActionValue;

UENUM(BlueprintType)
enum class EVoraxiaFocusInputMode : uint8
{
	Hold UMETA(DisplayName="Hold"),
	Toggle UMETA(DisplayName="Toggle")
};

UCLASS()
class VORAXIA_API AVoraxiaPlayerCharacter
	: public ACharacter
	, public IVoraxiaMiningInventoryReceiver
{
	GENERATED_BODY()

public:
	explicit AVoraxiaPlayerCharacter(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()
	);

	virtual void Tick(float DeltaTime) override;
	virtual void PawnClientRestart() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void ReceiveMiningYield_Implementation(
		const FVoraxiaMiningYield& MiningYield
	) override;

	UFUNCTION(BlueprintPure, Category = "Voraxia|Mining|Ledger")
	float GetMinedResourceAmount(FGameplayTag ResourceTag) const;

	UFUNCTION(BlueprintCallable, Category = "Voraxia|Mining|Ledger")
	void LogMiningLedger() const;

	/*
	 * Read-only resource totals for debug UI and future ship/station transfer
	 * systems. The inventory component is the gameplay source of truth.
	 */
	const TMap<FGameplayTag, float>& GetResourceInventory() const;

	/*
	 * Temporary compatibility accessor for the existing debug Slate widget.
	 * New gameplay code should use GetResourceInventory().
	 */
	const TMap<FGameplayTag, float>& GetMiningResourceLedger() const
	{
		return GetResourceInventory();
	}

	UFUNCTION(BlueprintPure, Category = "Voraxia|Inventory")
	UVoraxiaResourceInventoryComponent* GetResourceInventoryComponent() const
	{
		return ResourceInventoryComponent;
	}

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia|Camera")
	TObjectPtr<UVoraxiaCameraOcclusionDitherComponent> CameraOcclusionDitherComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia|Camera")
	TObjectPtr<UVoraxiaCameraComponent> VoraxiaCameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia|Mining")
	TObjectPtr<UVoraxiaRaptorMiningComponent> RaptorMiningComponent;
	

	/*
	 * Resource-only inventory for the prototype mining loop.
	 *
	 * It owns the resource totals that mining, future ship transfer, station
	 * storage, and exchange systems will use. Capacity, mass, persistence, and
	 * replicated fast-array support are deliberately deferred until the first
	 * complete inventory loop needs them.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Inventory")
	TObjectPtr<UVoraxiaResourceInventoryComponent> ResourceInventoryComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voraxia|Inventory|Debug")
	bool bLogMiningLedgerUpdates = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	int32 MappingContextPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	TObjectPtr<UInputAction> SprintAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	TObjectPtr<UInputAction> FocusAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	TObjectPtr<UInputAction> SwapShoulderAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voraxia|Input")
	TObjectPtr<UInputAction> MiningAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voraxia|Input")
	TObjectPtr<UInputAction> InteractAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Movement")
	bool bCharacterFacesCameraYaw = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Movement", meta=(EditCondition="bCharacterFacesCameraYaw"))
	float FaceCameraYawInterpSpeed = 16.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Movement|Sprint")
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Movement|Sprint")
	float SprintSpeed = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Movement|Sprint")
	float SprintSpeedInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Movement|Sprint")
	bool bCanSprint = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Camera|Focus")
	EVoraxiaFocusInputMode FocusInputMode = EVoraxiaFocusInputMode::Hold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Camera|Focus")
	float FocusBlendInTime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Camera|Focus")
	float FocusBlendOutTime = 0.25f;

private:
	void AddDefaultMappingContext();

	void AddMiningLedgerWidget();
	void RemoveMiningLedgerWidget();

	TSharedPtr<SVoraxiaMiningLedgerWidget> MiningLedgerWidget;

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);
	
	void UpdateCharacterFacing(float DeltaTime);
	
	void SprintStarted(const FInputActionValue& Value);
	void SprintEnded(const FInputActionValue& Value);
	void UpdateSprintSpeed(float DeltaTime);
	
	void SwapShoulderStarted(const FInputActionValue& Value);
	
	bool bWantsToSprint = false;
	bool bFocusToggleActive = false;

	
	void FocusStarted(const FInputActionValue& Value);
	void FocusEnded(const FInputActionValue& Value);
	
	void MiningStarted(const FInputActionValue& Value);
	void MiningEnded(const FInputActionValue& Value);
	
public:
	bool AddPhysicalBlueprint(UVoraxiaBlueprintDataAsset* BlueprintData);
	bool RemovePhysicalBlueprint(UVoraxiaBlueprintDataAsset* BlueprintData);
	bool HasPhysicalBlueprint(UVoraxiaBlueprintDataAsset* BlueprintData) const;

	const TArray<TObjectPtr<UVoraxiaBlueprintDataAsset>>& GetPhysicalBlueprints() const;
	
	UVoraxiaBlueprintDataAsset* GetFirstPhysicalBlueprint() const;
	
protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Voraxia|Blueprints")
	TArray<TObjectPtr<UVoraxiaBlueprintDataAsset>> PhysicalBlueprints;
	
protected:
	void TryInteract();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voraxia|Interaction")
	float InteractionTraceDistance = 300.0f;
};