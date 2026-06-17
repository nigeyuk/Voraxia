#pragma once

#include "CoreMinimal.h"


#include "VoraxiaLogCategories.h"
#include "Modules/ModuleManager.h"

class FVoraxiaLogModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
