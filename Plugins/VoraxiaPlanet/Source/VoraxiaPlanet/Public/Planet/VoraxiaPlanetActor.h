// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetActor.h
 * @brief Multiplayer-aware runtime actor representing one Voraxia planet.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Planet/VoraxiaPlanetDefinition.h"

#include "VoraxiaPlanetActor.generated.h"

class USceneComponent;
class UVoraxiaPlanetDebugComponent;
class UVoraxiaPlanetSurfacePatchComponent;

/**
 * @brief Runtime, multiplayer-aware representation of a Voraxia planet.
 *
 * The actor currently owns:
 *
 * - A local design-time planet definition asset.
 * - Compact server-authored runtime state replicated to clients.
 * - A local-only cube-sphere debug visualisation component.
 *
 * Terrain generation, chunk streaming, gravity application, terrain edits,
 * persistence, and replication relevance will be added around this stable
 * multiplayer foundation rather than replacing it later.
 */
UCLASS(BlueprintType)
class VORAXIAPLANET_API AVoraxiaPlanetActor : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * @brief Creates the planet actor and its core components.
	 */
	AVoraxiaPlanetActor();

	/**
	 * @brief Initialises server-owned runtime planet state from the definition asset.
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief Registers replicated properties for this actor.
	 *
	 * @param OutLifetimeProps Property lifetime list populated for Unreal networking.
	 */
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @brief Local design-time configuration asset assigned in the level.
	 *
	 * This asset is deliberately not replicated. The server validates it and
	 * replicates only compact runtime data to clients.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Voraxia Planet")
	TObjectPtr<UVoraxiaPlanetDefinition> PlanetDefinition;

	/**
	 * @brief Compact, validated state authored by the server and replicated to clients.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_PlanetRuntimeState,
		Category = "Voraxia Planet")
	FVoraxiaPlanetRuntimeState PlanetRuntimeState;

	/**
	 * @brief Returns this actor's design-time definition asset.
	 *
	 * @return Assigned planet definition, or null when none has been set.
	 */
	UFUNCTION(BlueprintPure, Category = "Voraxia Planet")
	UVoraxiaPlanetDefinition* GetPlanetDefinition() const
	{
		return PlanetDefinition;
	}

	/**
	 * @brief Returns the current server-authored runtime state.
	 *
	 * @return Current replicated runtime state.
	 */
	UFUNCTION(BlueprintPure, Category = "Voraxia Planet")
	FVoraxiaPlanetRuntimeState GetPlanetRuntimeState() const
	{
		return PlanetRuntimeState;
	}

	/**
	 * @brief Returns whether the actor currently has a valid runtime planet state.
	 *
	 * @return True after server initialisation or successful client replication.
	 */
	UFUNCTION(BlueprintPure, Category = "Voraxia Planet")
	bool HasValidPlanetRuntimeState() const
	{
		return PlanetRuntimeState.IsValid();
	}

protected:
	/**
	 * @brief Root scene component for future terrain, atmosphere, and streaming systems.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia Planet")
	TObjectPtr<USceneComponent> PlanetRoot;

	/**
	 * @brief Local-only visual debug component for cube-sphere diagnostics.
	 *
	 * It reads replicated state but does not replicate debug geometry itself.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voraxia Planet|Debug")
	TObjectPtr<UVoraxiaPlanetDebugComponent> PlanetDebugComponent;
	
	/**
	 * @brief Local Dynamic Mesh preview for one deterministic terrain patch.
	 *
	 * The component reconstructs its preview geometry from replicated planet state.
	 * It does not replicate vertices, triangles, collision, or render buffers.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Voraxia Planet|Terrain Preview")
	TObjectPtr<UVoraxiaPlanetSurfacePatchComponent>
		PlanetSurfacePatchPreviewComponent;
	
	/**
	 * @brief Called on clients when server-authored runtime state arrives.
	 */
	UFUNCTION()
	void OnRep_PlanetRuntimeState();

private:
	/**
	 * @brief Creates authoritative runtime state from the local definition asset.
	 *
	 * This function must only run on the server.
	 *
	 * @return True when the definition was valid and runtime state was created.
	 */
	bool InitialisePlanetRuntimeStateFromDefinition();

	/**
	 * @brief Writes a concise planet lifecycle diagnostic when enabled in settings.
	 *
	 * @param Context Description of the lifecycle event being logged.
	 */
	void LogPlanetRuntimeState(const TCHAR* Context) const;
};