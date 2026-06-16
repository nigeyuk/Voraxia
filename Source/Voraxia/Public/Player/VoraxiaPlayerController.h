// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VoraxiaPlayerController.generated.h"

UCLASS(Blueprintable)
class VORAXIA_API AVoraxiaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AVoraxiaPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
};
