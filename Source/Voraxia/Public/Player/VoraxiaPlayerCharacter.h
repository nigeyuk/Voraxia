#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VoraxiaPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UVoraxiaCameraComponent;

struct FInputActionValue;

UCLASS()
class VORAXIA_API AVoraxiaPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AVoraxiaPlayerCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void PawnClientRestart() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voraxia|Camera")
	TObjectPtr<UVoraxiaCameraComponent> VoraxiaCameraComponent;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Movement")
	bool bCharacterFacesCameraYaw = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Voraxia|Movement", meta=(EditCondition="bCharacterFacesCameraYaw"))
	float FaceCameraYawInterpSpeed = 16.0f;

private:
	void AddDefaultMappingContext();

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);
	
	void UpdateCharacterFacing(float DeltaTime);
	
};