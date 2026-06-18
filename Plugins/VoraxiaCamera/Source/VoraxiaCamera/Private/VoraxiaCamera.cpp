// Copyright 2026 Coding Custard Studios

#include "VoraxiaCamera.h"

#include "VoraxiaCameraLog.h"

#define LOCTEXT_NAMESPACE "FVoraxiaCameraRuntimeModule"

void FVoraxiaCameraModule::StartupModule()
{
	UE_LOG(LogVoraxiaCamera, Log, TEXT("VoraxiaCameraRuntime module started."));
}

void FVoraxiaCameraModule::ShutdownModule()
{
	UE_LOG(LogVoraxiaCamera, Log, TEXT("VoraxiaCameraRuntime module shut down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVoraxiaCameraModule, VoraxiaCamera)