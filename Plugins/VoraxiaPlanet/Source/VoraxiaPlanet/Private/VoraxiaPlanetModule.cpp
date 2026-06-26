#include "VoraxiaPlanetModule.h"

#include "Modules/ModuleManager.h"

void FVoraxiaPlanetModule::StartupModule()
{
    // Deliberately empty: this milestone proves the plugin boundary only.
}

void FVoraxiaPlanetModule::ShutdownModule()
{
    // Deliberately empty: no runtime-owned resources exist yet.
}

IMPLEMENT_MODULE(FVoraxiaPlanetModule, VoraxiaPlanet)
