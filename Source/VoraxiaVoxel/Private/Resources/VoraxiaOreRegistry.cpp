// Copyright 2026 Coding Custard Studios.

#include "Resources/VoraxiaOreRegistry.h"

#include "Resources/VoraxiaOreDefinition.h"
#include "VoraxiaLog.h"

const UVoraxiaOreDefinition* UVoraxiaOreRegistry::FindOreDefinition(
	const FGameplayTag OreTag
) const
{
	if (!OreTag.IsValid())
	{
		return nullptr;
	}

	for (const TObjectPtr<UVoraxiaOreDefinition>& OreDefinition
		: OreDefinitions)
	{
		if (!IsValid(OreDefinition))
		{
			continue;
		}

		if (OreDefinition->OreTag.MatchesTagExact(OreTag))
		{
			return OreDefinition;
		}
	}

	UE_LOG(
		LogVoraxiaVoxel,
		Warning,
		TEXT(
			"Ore registry '%s' could not resolve ore tag '%s'."
		),
		*GetName(),
		*OreTag.ToString()
	);

	return nullptr;
}

bool UVoraxiaOreRegistry::HasOreDefinition(
	const FGameplayTag OreTag
) const
{
	return FindOreDefinition(OreTag) != nullptr;
}