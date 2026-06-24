// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AVoraxiaPlayerCharacter;

/*
 * Lightweight top-right Slate overlay for the prototype mining inventory.
 *
 * This is intentionally a debug widget. It reads the owning player's
 * current resource inventory directly and does not own gameplay state.
 */
class VORAXIA_API SVoraxiaMiningLedgerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVoraxiaMiningLedgerWidget)
		: _OwningPlayer(nullptr)
	{
	}

	SLATE_ARGUMENT(TWeakObjectPtr<AVoraxiaPlayerCharacter>, OwningPlayer)

SLATE_END_ARGS()

void Construct(const FArguments& InArgs);

private:
	TWeakObjectPtr<AVoraxiaPlayerCharacter> OwningPlayer;

	FText GetInventoryText() const;
};
