// Copyright 2026 Coding Custard Studios.

#pragma once

#include "Modules/ModuleManager.h"

class FVoraxiaCameraModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
