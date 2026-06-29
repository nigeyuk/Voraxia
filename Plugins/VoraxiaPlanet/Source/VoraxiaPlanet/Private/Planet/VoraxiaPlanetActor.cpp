// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetActor.cpp
 * @brief Implementation of the multiplayer-aware Voraxia planet actor.
 */

#include "Planet/VoraxiaPlanetActor.h"

#include "Terrain/VoraxiaPlanetSurfacePatchComponent.h"
#include "Components/SceneComponent.h"
#include "Core/VoraxiaPlanetDeveloperSettings.h"
#include "Debug/VoraxiaPlanetDebugComponent.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogVoraxiaPlanet, Log, All);

namespace
{
	/**
	 * @brief Converts Unreal network mode to a compact log-friendly label.
	 *
	 * @param World World whose networking mode should be described.
	 *
	 * @return Text describing the current network mode.
	 */
	const TCHAR* GetNetModeLabel(const UWorld* World)
	{
		if (!IsValid(World))
		{
			return TEXT("NoWorld");
		}

		switch (World->GetNetMode())
		{
		case NM_Standalone:
			return TEXT("Standalone");

		case NM_DedicatedServer:
			return TEXT("DedicatedServer");

		case NM_ListenServer:
			return TEXT("ListenServer");

		case NM_Client:
			return TEXT("Client");

		default:
			return TEXT("UnknownNetMode");
		}
	}

	/**
	 * @brief Converts an Unreal local network role to a log-friendly label.
	 *
	 * @param LocalRole Actor role to describe.
	 *
	 * @return Text describing the local role.
	 */
	const TCHAR* GetLocalRoleLabel(const ENetRole LocalRole)
	{
		switch (LocalRole)
		{
		case ROLE_None:
			return TEXT("None");

		case ROLE_SimulatedProxy:
			return TEXT("SimulatedProxy");

		case ROLE_AutonomousProxy:
			return TEXT("AutonomousProxy");

		case ROLE_Authority:
			return TEXT("Authority");

		default:
			return TEXT("UnknownRole");
		}
	}
}

AVoraxiaPlanetActor::AVoraxiaPlanetActor()
{
	PlanetRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlanetRoot"));
	SetRootComponent(PlanetRoot);

	PlanetDebugComponent =
		CreateDefaultSubobject<UVoraxiaPlanetDebugComponent>(
			TEXT("PlanetDebugComponent"));

	/**
	* @brief Creates the local Dynamic Mesh terrain-patch preview component.
	 *
	 * This component is not replicated. It rebuilds the same visual patch locally
	 * from the replicated planet runtime state received from the server.
	*/
	PlanetSurfacePatchPreviewComponent =
		CreateDefaultSubobject<UVoraxiaPlanetSurfacePatchComponent>(
			TEXT("PlanetSurfacePatchPreviewComponent"));

	PlanetSurfacePatchPreviewComponent->SetupAttachment(PlanetRoot);
	
	/**
	 * Multiplayer-first actor configuration.
	 *
	 * The actor itself remains in a stable place. Future planet-local origin
	 * and terrain streaming systems will manage local presentation around it,
	 * so normal actor movement replication is intentionally disabled.
	 */
	bReplicates = true;
	bAlwaysRelevant = true;
	bNetLoadOnClient = true;

	SetReplicateMovement(false);
}

#if WITH_EDITOR
void AVoraxiaPlanetActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const UWorld* World = GetWorld();

	if (!IsValid(World) || World->IsGameWorld())
	{
		return;
	}

	RefreshEditorPreviewRuntimeState();
	RebuildTerrainPreview();
}
#endif

void AVoraxiaPlanetActor::RebuildTerrainPreview()
{
	if (IsValid(PlanetSurfacePatchPreviewComponent))
	{
		PlanetSurfacePatchPreviewComponent->RebuildPreviewMesh();
	}
}

#if WITH_EDITOR
void AVoraxiaPlanetActor::RefreshEditorPreviewRuntimeState()
{
#if WITH_EDITORONLY_DATA
	EditorPreviewRuntimeState = FVoraxiaPlanetRuntimeState();
	EditorPreviewFeatureProfile = nullptr;
	EditorPreviewValidationStatus =
		TEXT("No Planet Definition has been assigned.");

	if (!IsValid(PlanetDefinition))
	{
		return;
	}

	EditorPreviewFeatureProfile = PlanetDefinition->FeatureProfile;

	FString ValidationFailureReason;

	if (!PlanetDefinition->IsDefinitionValid(ValidationFailureReason))
	{
		EditorPreviewValidationStatus = FString::Printf(
			TEXT("Definition is invalid: %s"),
			*ValidationFailureReason);

		return;
	}

	EditorPreviewRuntimeState = PlanetDefinition->CreateRuntimeState();
	EditorPreviewValidationStatus =
		TEXT("Valid editor preview. Runtime state remains server-authoritative during play.");
#endif
}

void AVoraxiaPlanetActor::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName =
		PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(
		AVoraxiaPlanetActor,
		PlanetDefinition))
	{
		RefreshEditorPreviewRuntimeState();
		RebuildTerrainPreview();
	}
}
#endif

void AVoraxiaPlanetActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (!InitialisePlanetRuntimeStateFromDefinition())
		{
			UE_LOG(
				LogVoraxiaPlanet,
				Error,
				TEXT("[%s] Planet actor could not initialise authoritative runtime state."),
				*GetName());

			return;
		}

		/**
		 * Runtime state is generated during BeginPlay, so request a timely
		 * replication update rather than waiting for the ordinary net cadence.
		 */
		ForceNetUpdate();
	}
	else if (PlanetRuntimeState.IsValid())
	{
		/**
		 * A placed actor can sometimes receive initial replicated state before
		 * its client BeginPlay finishes.
		 *
		 * Refresh the local preview here as well as in OnRep so clients cannot
		 * miss the mesh build when replication arrives unusually early.
		 */
		RebuildTerrainPreview();

		LogPlanetRuntimeState(TEXT("Client BeginPlay"));
	}
}

void AVoraxiaPlanetActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVoraxiaPlanetActor, PlanetRuntimeState);
}

void AVoraxiaPlanetActor::OnRep_PlanetRuntimeState()
{
	if (!PlanetRuntimeState.IsValid())
	{
		UE_LOG(
			LogVoraxiaPlanet,
			Warning,
			TEXT("[%s] Received an invalid replicated planet runtime state."),
			*GetName());

		return;
	}

	/**
	 * The client now has authoritative planet identity, radius, and generator
	 * version. Rebuild the local preview mesh from that replicated state.
	 *
	 * No vertices or triangles are replicated here.
	 */
	RebuildTerrainPreview();

	LogPlanetRuntimeState(TEXT("Client OnRep"));
}

bool AVoraxiaPlanetActor::InitialisePlanetRuntimeStateFromDefinition()
{
	if (!HasAuthority())
	{
		UE_LOG(
			LogVoraxiaPlanet,
			Warning,
			TEXT("[%s] Tried to initialise planet state without server authority."),
			*GetName());

		return false;
	}

	if (!IsValid(PlanetDefinition))
	{
		UE_LOG(
			LogVoraxiaPlanet,
			Error,
			TEXT("[%s] No PlanetDefinition has been assigned."),
			*GetName());

		return false;
	}

	FString ValidationFailureReason;

	if (!PlanetDefinition->IsDefinitionValid(ValidationFailureReason))
	{
		UE_LOG(
			LogVoraxiaPlanet,
			Error,
			TEXT("[%s] PlanetDefinition '%s' is invalid: %s"),
			*GetName(),
			*PlanetDefinition->GetName(),
			*ValidationFailureReason);

		return false;
	}

	PlanetRuntimeState = PlanetDefinition->CreateRuntimeState();

	/**
	 * The server also creates its own local preview from the same authoritative
	 * runtime state it will replicate to connected clients.
	 */
	RebuildTerrainPreview();

	LogPlanetRuntimeState(TEXT("Server Initialised"));

	return true;
}

void AVoraxiaPlanetActor::LogPlanetRuntimeState(
	const TCHAR* Context) const
{
	const UVoraxiaPlanetDeveloperSettings* Settings =
		GetDefault<UVoraxiaPlanetDeveloperSettings>();

	if (IsValid(Settings) && !Settings->bLogPlanetLifecycle)
	{
		return;
	}

	const double RadiusKilometres =
		VoraxiaPlanetUnits::MetresToKilometres(
			PlanetRuntimeState.RadiusMetres);

	const UWorld* World = GetWorld();

	const FString WorldName = IsValid(World)
		? World->GetMapName()
		: TEXT("NoWorld");

	UE_LOG(
		LogVoraxiaPlanet,
		Log,
		TEXT("[%s] %s | World=%s | NetMode=%s | Role=%s | Definition=%s | PlanetId=%s | Seed=%d | GeneratorVersion=%d | FeatureProfile=%s | FeatureProfileVersion=%d | Radius=%.2f km"),
		*GetName(),
		Context,
		*WorldName,
		GetNetModeLabel(World),
		GetLocalRoleLabel(GetLocalRole()),
		*PlanetRuntimeState.DefinitionName.ToString(),
		*PlanetRuntimeState.PlanetId.Value.ToString(
			EGuidFormats::DigitsWithHyphens),
		PlanetRuntimeState.Seed,
		PlanetRuntimeState.GeneratorVersion,
		*PlanetRuntimeState.FeatureProfileId.ToString(),
		PlanetRuntimeState.FeatureProfileVersion,
		RadiusKilometres);
}