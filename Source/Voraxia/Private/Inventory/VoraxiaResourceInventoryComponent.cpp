// Copyright 2026 Coding Custard Studios.

#include "Inventory/VoraxiaResourceInventoryComponent.h"

UVoraxiaResourceInventoryComponent::UVoraxiaResourceInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UVoraxiaResourceInventoryComponent::AddResource(
	const FGameplayTag ResourceTag,
	const float Amount
)
{
	if (!ResourceTag.IsValid() || Amount <= KINDA_SMALL_NUMBER)
	{
		return GetResourceAmount(ResourceTag);
	}

	float& StoredAmount = ResourceAmounts.FindOrAdd(ResourceTag);
	StoredAmount += Amount;

	OnResourceAmountChanged.Broadcast(
		ResourceTag,
		Amount,
		StoredAmount
	);

	return StoredAmount;
}

float UVoraxiaResourceInventoryComponent::RemoveResource(
	const FGameplayTag ResourceTag,
	const float Amount,
	const bool bAllowPartialRemoval
)
{
	if (!ResourceTag.IsValid() || Amount <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	float* StoredAmount = ResourceAmounts.Find(ResourceTag);

	if (!StoredAmount || *StoredAmount <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	if (!bAllowPartialRemoval && *StoredAmount + KINDA_SMALL_NUMBER < Amount)
	{
		return 0.0f;
	}

	const float RemovedAmount = bAllowPartialRemoval
		? FMath::Min(*StoredAmount, Amount)
		: Amount;

	*StoredAmount = FMath::Max(0.0f, *StoredAmount - RemovedAmount);

	const float NewAmount = *StoredAmount;

	if (NewAmount <= KINDA_SMALL_NUMBER)
	{
		ResourceAmounts.Remove(ResourceTag);
	}

	OnResourceAmountChanged.Broadcast(
		ResourceTag,
		-RemovedAmount,
		NewAmount
	);

	return RemovedAmount;
}

float UVoraxiaResourceInventoryComponent::GetResourceAmount(
	const FGameplayTag ResourceTag
) const
{
	if (const float* StoredAmount = ResourceAmounts.Find(ResourceTag))
	{
		return *StoredAmount;
	}

	return 0.0f;
}

bool UVoraxiaResourceInventoryComponent::HasResource(
	const FGameplayTag ResourceTag,
	const float MinimumAmount
) const
{
	return GetResourceAmount(ResourceTag) >=
		FMath::Max(MinimumAmount, KINDA_SMALL_NUMBER);
}

void UVoraxiaResourceInventoryComponent::ClearResource(
	const FGameplayTag ResourceTag
)
{
	const float RemovedAmount = GetResourceAmount(ResourceTag);

	if (RemovedAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	ResourceAmounts.Remove(ResourceTag);

	OnResourceAmountChanged.Broadcast(
		ResourceTag,
		-RemovedAmount,
		0.0f
	);
}

void UVoraxiaResourceInventoryComponent::ClearAllResources()
{
	TArray<TPair<FGameplayTag, float>> Entries;
	Entries.Reserve(ResourceAmounts.Num());

	for (const TPair<FGameplayTag, float>& Entry : ResourceAmounts)
	{
		Entries.Add(Entry);
	}

	ResourceAmounts.Reset();

	for (const TPair<FGameplayTag, float>& Entry : Entries)
	{
		OnResourceAmountChanged.Broadcast(
			Entry.Key,
			-Entry.Value,
			0.0f
		);
	}
}
