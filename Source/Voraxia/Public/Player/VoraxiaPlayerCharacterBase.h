// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VoraxiaPlayerCharacterBase.generated.h"

UCLASS(Abstract, Blueprintable)
class VORAXIA_API AVoraxiaPlayerCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AVoraxiaPlayerCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
};
