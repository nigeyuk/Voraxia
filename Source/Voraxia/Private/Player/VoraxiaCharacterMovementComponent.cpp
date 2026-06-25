// Copyright 2026 Coding Custard Studios.

#include "Player/VoraxiaCharacterMovementComponent.h"

#include "GameFramework/Character.h"
#include "Engine/PackageMapClient.h"

namespace VoraxiaCharacterMovement
{
	constexpr float YawPackingScale = 65535.0f / 360.0f;
	constexpr float YawUnpackingScale = 360.0f / 65535.0f;

	uint16 PackFacingYaw(const float FacingYaw)
	{
		const float WrappedYaw = FRotator::ClampAxis(FacingYaw);
		return static_cast<uint16>(FMath::RoundToInt(WrappedYaw * YawPackingScale));
	}

	float UnpackFacingYaw(const uint16 PackedFacingYaw)
	{
		return FRotator::NormalizeAxis(
			static_cast<float>(PackedFacingYaw) * YawUnpackingScale
		);
	}
}

class FNetworkPredictionData_Client_VoraxiaCharacter final
	: public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_VoraxiaCharacter(
		const UCharacterMovementComponent& ClientMovement
	)
		: FNetworkPredictionData_Client_Character(ClientMovement)
	{
	}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return FSavedMovePtr(new FSavedMove_VoraxiaCharacter());
	}
};

void FSavedMove_VoraxiaCharacter::Clear()
{
	FSavedMove_Character::Clear();
	PackedFacingYaw = 0;
}

bool FSavedMove_VoraxiaCharacter::CanCombineWith(
	const FSavedMovePtr& NewMove,
	ACharacter* Character,
	const float MaxDelta
) const
{
	if (!FSavedMove_Character::CanCombineWith(NewMove, Character, MaxDelta))
	{
		return false;
	}

	const FSavedMove_VoraxiaCharacter* NewVoraxiaMove =
		static_cast<const FSavedMove_VoraxiaCharacter*>(NewMove.Get());

	/*
	 * Do not merge two moves when their visual body facing differs.  The yaw is
	 * inexpensive, and retaining it keeps server movement reconstruction aligned
	 * with the local camera-facing state during a run-and-turn.
	 */
	return NewVoraxiaMove &&
		NewVoraxiaMove->PackedFacingYaw == PackedFacingYaw;
}

void FSavedMove_VoraxiaCharacter::SetMoveFor(
	ACharacter* Character,
	const float InDeltaTime,
	FVector const& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData
)
{
	FSavedMove_Character::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	const UVoraxiaCharacterMovementComponent* VoraxiaMovement =
		Character
			? Cast<UVoraxiaCharacterMovementComponent>(
				Character->GetCharacterMovement()
			)
			: nullptr;

	PackedFacingYaw = VoraxiaMovement
		? VoraxiaCharacterMovement::PackFacingYaw(
			VoraxiaMovement->GetVoraxiaFacingYaw()
		)
		: 0;
}

void FSavedMove_VoraxiaCharacter::PrepMoveFor(ACharacter* Character)
{
	FSavedMove_Character::PrepMoveFor(Character);

	if (UVoraxiaCharacterMovementComponent* VoraxiaMovement =
		Character
			? Cast<UVoraxiaCharacterMovementComponent>(
				Character->GetCharacterMovement()
			)
			: nullptr)
	{
		/*
		 * Server corrections replay unacknowledged saved moves on the owning
		 * client.  Reapply the yaw saved with each move before its replay so the
		 * custom camera-facing body stays in lockstep with the prediction queue.
		 */
		VoraxiaMovement->ApplyVoraxiaFacingYaw(
			VoraxiaCharacterMovement::UnpackFacingYaw(PackedFacingYaw)
		);
	}
}

void FVoraxiaCharacterNetworkMoveData::ClientFillNetworkMoveData(
	const FSavedMove_Character& ClientMove,
	const ENetworkMoveType MoveType
)
{
	FCharacterNetworkMoveData::ClientFillNetworkMoveData(
		ClientMove,
		MoveType
	);

	const FSavedMove_VoraxiaCharacter& VoraxiaMove =
		static_cast<const FSavedMove_VoraxiaCharacter&>(ClientMove);

	PackedFacingYaw = VoraxiaMove.PackedFacingYaw;
}

bool FVoraxiaCharacterNetworkMoveData::Serialize(
	UCharacterMovementComponent& CharacterMovement,
	FArchive& Ar,
	UPackageMap* PackageMap,
	const ENetworkMoveType MoveType
)
{
	if (!FCharacterNetworkMoveData::Serialize(CharacterMovement, Ar, PackageMap, MoveType))
	{
		return false;
	}

	Ar.SerializeBits(&PackedFacingYaw, 16);
	return !Ar.IsError();
}

float FVoraxiaCharacterNetworkMoveData::GetFacingYaw() const
{
	return VoraxiaCharacterMovement::UnpackFacingYaw(PackedFacingYaw);
}

FVoraxiaCharacterNetworkMoveDataContainer::
FVoraxiaCharacterNetworkMoveDataContainer()
{
	NewMoveData = &VoraxiaNewMoveData;
	PendingMoveData = &VoraxiaPendingMoveData;
	OldMoveData = &VoraxiaOldMoveData;
}

UVoraxiaCharacterMovementComponent::UVoraxiaCharacterMovementComponent()
{
	SetNetworkMoveDataContainer(VoraxiaMoveDataContainer);
}

void UVoraxiaCharacterMovementComponent::SetVoraxiaFacingYaw(
	const float NewFacingYaw
)
{
	if (FMath::IsFinite(NewFacingYaw))
	{
		VoraxiaFacingYaw = FRotator::NormalizeAxis(NewFacingYaw);
	}
}

float UVoraxiaCharacterMovementComponent::GetVoraxiaFacingYaw() const
{
	return VoraxiaFacingYaw;
}

void UVoraxiaCharacterMovementComponent::ApplyVoraxiaFacingYaw(
	const float NewFacingYaw
)
{
	SetVoraxiaFacingYaw(NewFacingYaw);

	if (!CharacterOwner)
	{
		return;
	}

	FRotator UpdatedRotation = CharacterOwner->GetActorRotation();
	UpdatedRotation.Yaw = VoraxiaFacingYaw;
	CharacterOwner->SetActorRotation(UpdatedRotation);
}

FNetworkPredictionData_Client*
UVoraxiaCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UVoraxiaCharacterMovementComponent* MutableThis =
			const_cast<UVoraxiaCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData =
			new FNetworkPredictionData_Client_VoraxiaCharacter(*this);
	}

	return ClientPredictionData;
}

void UVoraxiaCharacterMovementComponent::MoveAutonomous(
	const float ClientTimeStamp,
	const float DeltaTime,
	const uint8 CompressedFlags,
	const FVector& NewAccel
)
{
	/*
	 * The server processes the same facing yaw that was saved with this exact
	 * movement move.  This replaces the standalone yaw RPC and prevents a
	 * delayed, independent transform update from fighting movement prediction.
	 */
	if (const FVoraxiaCharacterNetworkMoveData* VoraxiaMoveData =
		static_cast<const FVoraxiaCharacterNetworkMoveData*>(
			GetCurrentNetworkMoveData()
		))
	{
		ApplyVoraxiaFacingYaw(VoraxiaMoveData->GetFacingYaw());
	}

	UCharacterMovementComponent::MoveAutonomous(
		ClientTimeStamp,
		DeltaTime,
		CompressedFlags,
		NewAccel
	);
}
