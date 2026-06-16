// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Player/VoraxiaPlayerCharacterBase.h"
#include "VoraxiaPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS(Blueprintable)
class VORAXIA_API AVoraxiaPlayerCharacter : public AVoraxiaPlayerCharacterBase
{
	GENERATED_BODY()

public:
	AVoraxiaPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
