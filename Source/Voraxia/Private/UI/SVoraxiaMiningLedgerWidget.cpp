// Copyright 2026 Coding Custard Studios.

#include "UI/SVoraxiaMiningLedgerWidget.h"

#include "Player/VoraxiaPlayerCharacter.h"

#include "VoraxiaBlueprintDataAsset.h"
#include "Algo/Sort.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void SVoraxiaMiningLedgerWidget::Construct(const FArguments& InArgs)
{
	OwningPlayer = InArgs._OwningPlayer;

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 32.0f, 32.0f, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.78f))
			.Padding(FMargin(14.0f, 12.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 7.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("MINING INVENTORY")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
					.ColorAndOpacity(FLinearColor::White)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SVoraxiaMiningLedgerWidget::GetInventoryText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
					.ColorAndOpacity(FLinearColor(0.92f, 0.92f, 0.92f, 1.0f))
				]
			]
		]
	];
}

FText SVoraxiaMiningLedgerWidget::GetInventoryText() const
{
	const AVoraxiaPlayerCharacter* Player = OwningPlayer.Get();

	if (!Player)
	{
		return FText::FromString(TEXT("Awaiting player inventory..."));
	}

	FString InventoryLines;

	const TMap<FGameplayTag, float>& Inventory =
		Player->GetResourceInventory();

	InventoryLines += TEXT("RESOURCES\n");

	if (Inventory.IsEmpty())
	{
		InventoryLines += TEXT("- None\n");
	}
	else
	{
		TArray<TPair<FGameplayTag, float>> SortedEntries;
		SortedEntries.Reserve(Inventory.Num());

		for (const TPair<FGameplayTag, float>& Entry : Inventory)
		{
			SortedEntries.Add(Entry);
		}

		SortedEntries.Sort(
			[](const TPair<FGameplayTag, float>& Left,
				const TPair<FGameplayTag, float>& Right)
			{
				return Left.Key.ToString() < Right.Key.ToString();
			}
		);

		for (const TPair<FGameplayTag, float>& Entry : SortedEntries)
		{
			FString ResourceName = Entry.Key.ToString();

			int32 LastSeparatorIndex = INDEX_NONE;

			if (ResourceName.FindLastChar(TEXT('.'), LastSeparatorIndex))
			{
				ResourceName = ResourceName.Mid(LastSeparatorIndex + 1);
			}

			InventoryLines += FString::Printf(
				TEXT("- %-14s %8.2f\n"),
				*ResourceName,
				Entry.Value
			);
		}
	}

	InventoryLines += TEXT("\nBLUEPRINTS\n");

	const TArray<TObjectPtr<UVoraxiaBlueprintDataAsset>>& Blueprints =
		Player->GetPhysicalBlueprints();

	if (Blueprints.IsEmpty())
	{
		InventoryLines += TEXT("- None");
	}
	else
	{
		for (const UVoraxiaBlueprintDataAsset* Blueprint : Blueprints)
		{
			if (!Blueprint)
			{
				continue;
			}

			InventoryLines += FString::Printf(
				TEXT("- %s\n"),
				*Blueprint->DisplayName.ToString()
			);
		}

		InventoryLines.TrimEndInline();
	}

	return FText::FromString(InventoryLines);
}

