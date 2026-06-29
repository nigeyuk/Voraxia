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
struct FPropertyChangedEvent;

/**
 * @brief Runtime, multiplayer-aware representation of a Voraxia planet.
 *
 * The actor currently owns:
 *
 * - A local design-time planet definition asset.
 * - Compact server-authored runtime state replicated to clients.
 * - A server-resolved immutable feature-profile contract in that runtime state.
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

#if WITH_EDITOR
	/**
	 * @brief Rebuilds non-authoritative editor preview data when the actor is constructed.
	 *
	 * @param Transform Actor transform supplied by Unreal's construction process.
	 */
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

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

#if WITH_EDITORONLY_DATA
	/**
	 * @brief Non-authoritative runtime state derived from PlanetDefinition for editor inspection.
	 *
	 * This field exists solely to make the resolved feature-profile contract visible
	 * before PIE. It is never replicated, saved as planet runtime state, or used by
	 * gameplay. The authoritative PlanetRuntimeState remains server-created in BeginPlay.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Transient,
		Category = "Voraxia Planet|Editor Preview")
	FVoraxiaPlanetRuntimeState EditorPreviewRuntimeState;

	/**
	 * @brief Direct editor-only view of the feature profile selected by PlanetDefinition.
	 *
	 * This is read-only and is refreshed from the assigned definition. Assign the
	 * profile on the definition asset, not on this actor.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Transient,
		Category = "Voraxia Planet|Editor Preview")
	TObjectPtr<UVoraxiaPlanetFeatureProfile> EditorPreviewFeatureProfile;

	/**
	 * @brief Editor-only explanation of the current preview contract state.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Transient,
		Category = "Voraxia Planet|Editor Preview")
	FString EditorPreviewValidationStatus;
#endif

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

	/**
	 * @brief Rebuilds the local terrain patch preview from runtime or definition data.
	 *
	 * In the editor this provides a manual Details-panel control. During play,
	 * the same function rebuilds from the server-authored replicated runtime state.
	 *
	 * This never changes replicated planet state or persistent terrain data.
	 */
	UFUNCTION(
		CallInEditor,
		Category = "Voraxia Planet|Terrain Preview")
	void RebuildTerrainPreview();

protected:
#if WITH_EDITOR
	/**
	 * @brief Refreshes editor preview state and geometry when the definition changes.
	 *
	 * @param PropertyChangedEvent Description of the edited Details-panel property.
	 */
	virtual void PostEditChangeProperty(
		FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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
#if WITH_EDITOR
	/**
	 * @brief Resolves an editor-only, non-authoritative preview contract from PlanetDefinition.
	 *
	 * The result is for Details-panel inspection only. It must not replace or mutate
	 * the server-authored PlanetRuntimeState.
	 */
	void RefreshEditorPreviewRuntimeState();
#endif

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