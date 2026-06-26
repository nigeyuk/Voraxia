// Copyright 2026 Coding Custard Studios.

#pragma once

#include "Modules/ModuleInterface.h"

/**
 * Runtime entry point for the VoraxiaPlanet plugin.
 *
 * This initial module intentionally contains no terrain implementation. Its job is
 * to establish a clean, independently compilable plugin boundary before planetary,
 * streaming, and networking systems are introduced.
 */
class FVoraxiaPlanetModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
