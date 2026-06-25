// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/CharacterMovementReplication.h"
#include "VoraxiaCharacterMovementComponent.generated.h"

class UPackageMap;

/*
 * Saved with each locally predicted Character Movement move.  The yaw is
 * deliberately independent from ControlRotation because Voraxia's camera is
 * an intentionally local presentation system.
 */
class FSavedMove_VoraxiaCharacter final : public FSavedMove_Character
{
public:
	uint16 PackedFacingYaw = 0;

	virtual void Clear() override;

	virtual bool CanCombineWith(
		const FSavedMovePtr& NewMove,
		ACharacter* Character,
		float MaxDelta
	) const override;

	virtual void SetMoveFor(
		ACharacter* Character,
		float InDeltaTime,
		FVector const& NewAccel,
		FNetworkPredictionData_Client_Character& ClientData
	) override;

	virtual void PrepMoveFor(ACharacter* Character) override;
};

/*
 * Packed alongside Unreal's normal client-to-server character move data.
 * This is not a standalone RPC and it does not carry any camera transform.
 */
struct FVoraxiaCharacterNetworkMoveData final : public FCharacterNetworkMoveData
{
	uint16 PackedFacingYaw = 0;

	virtual void ClientFillNetworkMoveData(
		const FSavedMove_Character& ClientMove,
		ENetworkMoveType MoveType
	) override;

	virtual bool Serialize(
		UCharacterMovementComponent& CharacterMovement,
		FArchive& Ar,
		UPackageMap* PackageMap,
		ENetworkMoveType MoveType
	) override;

	float GetFacingYaw() const;
};

/*
 * Storage for the new, pending, and old packed move records used by Unreal's
 * existing movement RPC pathway.
 */
struct FVoraxiaCharacterNetworkMoveDataContainer final
	: public FCharacterNetworkMoveDataContainer
{
	FVoraxiaCharacterNetworkMoveData VoraxiaNewMoveData;
	FVoraxiaCharacterNetworkMoveData VoraxiaPendingMoveData;
	FVoraxiaCharacterNetworkMoveData VoraxiaOldMoveData;

	FVoraxiaCharacterNetworkMoveDataContainer();
};

/*
 * Network extension for Voraxia's camera-driven body facing.
 *
 * It preserves all existing Character Movement and camera settings.  The only
 * added state is a compact facing yaw that travels in the same saved-move
 * packet as the corresponding movement input.
 */
UCLASS()
class VORAXIA_API UVoraxiaCharacterMovementComponent
	: public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UVoraxiaCharacterMovementComponent();

	void SetVoraxiaFacingYaw(float NewFacingYaw);
	float GetVoraxiaFacingYaw() const;

	void ApplyVoraxiaFacingYaw(float NewFacingYaw);

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual void MoveAutonomous(
		float ClientTimeStamp,
		float DeltaTime,
		uint8 CompressedFlags,
		const FVector& NewAccel
	) override;

private:
	float VoraxiaFacingYaw = 0.0f;

	FVoraxiaCharacterNetworkMoveDataContainer VoraxiaMoveDataContainer;
};
