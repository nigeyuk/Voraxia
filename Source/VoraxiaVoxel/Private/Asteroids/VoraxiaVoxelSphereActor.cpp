// Copyright 2026 Coding Custard Studios.

#include "Asteroids/VoraxiaVoxelSphereActor.h"

#include "Resources/VoraxiaOreDefinition.h"
#include "Resources/VoraxiaOreRegistry.h"
#include "Resources/VoraxiaOreTags.h"
#include "Meshing/VoraxiaMarchingCubes.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Mining/VoraxiaMiningTypes.h"
#include "Mining/VoraxiaVoxelMineable.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "VoraxiaLog.h"

AVoraxiaVoxelSphereActor::AVoraxiaVoxelSphereActor()
{
	PrimaryActorTick.bCanEverTick = false;

	VoxelMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("VoxelMesh"));
	SetRootComponent(VoxelMesh);

	/*
	 * Movable means the mesh topology may change during mining.
	 * It does not make the actor physically simulated.
	 */
	VoxelMesh->SetMobility(EComponentMobility::Movable);

	VoxelMesh->SetCollisionProfileName(TEXT("BlockAll"));
	VoxelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	VoxelMesh->SetSimulatePhysics(false);
	VoxelMesh->SetEnableGravity(false);
	
	BaseMaterialTag = TAG_VoraxiaResource_Ore_Rock;

	TestOreTags.Add(TAG_VoraxiaResource_Ore_Iron);
	TestOreTags.Add(TAG_VoraxiaResource_Ore_Copper);
}

void AVoraxiaVoxelSphereActor::BeginPlay()
{
	Super::BeginPlay();

	/*
	 * Reassert these in case a placed actor instance was changed in Details.
	 */
	VoxelMesh->SetSimulatePhysics(false);
	VoxelMesh->SetEnableGravity(false);

	if (bBuildOnBeginPlay)
	{
		BuildVoxelSphere();
	}
}

bool AVoraxiaVoxelSphereActor::ApplyVoxelCarve_Implementation(
	const FHitResult& Hit,
	const float WorldBrushRadius
)
{
	TArray<FVoraxiaMiningYield> IgnoredYields;

	return ApplyVoxelCarveWithYields_Implementation(
		Hit,
		WorldBrushRadius,
		IgnoredYields
	);
}

bool AVoraxiaVoxelSphereActor::ApplyVoxelCarveWithYields_Implementation(
	const FHitResult& Hit,
	const float WorldBrushRadius,
	TArray<FVoraxiaMiningYield>& OutYields
)
{
	OutYields.Reset();

	if (!Hit.bBlockingHit || WorldBrushRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	return CarveSphereAtWorldLocationInternal(
		Hit.ImpactPoint,
		WorldBrushRadius,
		OutYields
	);
}

void AVoraxiaVoxelSphereActor::BuildVoxelSphere()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			DetachedFragmentCleanupTimerHandle
		);
	}

	CellsPerAxis = FMath::Clamp(CellsPerAxis, 8, 64);
	VoxelSize = FMath::Max(VoxelSize, 1.0f);

	const float HalfExtent =
		static_cast<float>(CellsPerAxis) * VoxelSize * 0.5f;

	/*
	 * Keep the sphere one full voxel away from the density-grid border.
	 * This guarantees a closed surface instead of a sphere clipped by the box.
	 */
	const float LargestClosedSphereRadius =
		FMath::Max(VoxelSize, HalfExtent - VoxelSize);

	SphereRadius = FMath::Clamp(
		SphereRadius,
		VoxelSize,
		LargestClosedSphereRadius
	);

	GenerateSphereDensity();
	GenerateOreData();

	if (RebuildMeshFromDensity())
	{
		DrawOreDebugCells();
	}
}

void AVoraxiaVoxelSphereActor::RebuildSphereWithOre()
{
	BuildVoxelSphere();
}

bool AVoraxiaVoxelSphereActor::CarveSphereAtWorldLocation(
	const FVector& WorldLocation,
	const float WorldRadius
)
{
	TArray<FVoraxiaMiningYield> IgnoredYields;

	return CarveSphereAtWorldLocationInternal(
		WorldLocation,
		WorldRadius,
		IgnoredYields
	);
}

bool AVoraxiaVoxelSphereActor::CarveSphereAtWorldLocationInternal(
	const FVector& WorldLocation,
	const float WorldRadius,
	TArray<FVoraxiaMiningYield>& OutYields
)
{
	if (!VoxelMesh || WorldRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FTransform ComponentTransform = VoxelMesh->GetComponentTransform();

	const FVector Scale = ComponentTransform.GetScale3D();

	const float LargestScale = FMath::Max(
		FMath::Abs(Scale.X),
		FMath::Max(
			FMath::Abs(Scale.Y),
			FMath::Abs(Scale.Z)
		)
	);

	if (LargestScale <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector LocalLocation =
		ComponentTransform.InverseTransformPosition(WorldLocation);

	const float LocalRadius = WorldRadius / LargestScale;

	return CarveSphereAtLocalPositionInternal(
		LocalLocation,
		LocalRadius,
		OutYields
	);
}

bool AVoraxiaVoxelSphereActor::CarveSphereAtLocalPosition(
	const FVector& LocalLocation,
	const float LocalRadius
)
{
	TArray<FVoraxiaMiningYield> IgnoredYields;

	return CarveSphereAtLocalPositionInternal(
		LocalLocation,
		LocalRadius,
		IgnoredYields
	);
}

bool AVoraxiaVoxelSphereActor::CarveSphereAtLocalPositionInternal(
	const FVector& LocalLocation,
	const float LocalRadius,
	TArray<FVoraxiaMiningYield>& OutYields
)
{
	if (LocalRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	/*
	 * A mining edit can be called before BeginPlay by later tools or tests.
	 * Make sure a valid sphere density field exists first.
	 */
	if (!HasValidDensityField())
	{
		BuildVoxelSphere();

		if (!HasValidDensityField())
		{
			return false;
		}
	}

	const int32 SampleCount = CellsPerAxis + 1;

	const float HalfExtent =
		static_cast<float>(CellsPerAxis) * VoxelSize * 0.5f;

	/*
	 * Only inspect samples that could possibly be inside the brush.
	 * The extra one-sample border makes the edit robust around the brush edge.
	 */
	const int32 MinX = FMath::Clamp(
		FMath::FloorToInt(
			((LocalLocation.X - LocalRadius) + HalfExtent) / VoxelSize
		) - 1,
		0,
		CellsPerAxis
	);

	const int32 MaxX = FMath::Clamp(
		FMath::CeilToInt(
			((LocalLocation.X + LocalRadius) + HalfExtent) / VoxelSize
		) + 1,
		0,
		CellsPerAxis
	);

	const int32 MinY = FMath::Clamp(
		FMath::FloorToInt(
			((LocalLocation.Y - LocalRadius) + HalfExtent) / VoxelSize
		) - 1,
		0,
		CellsPerAxis
	);

	const int32 MaxY = FMath::Clamp(
		FMath::CeilToInt(
			((LocalLocation.Y + LocalRadius) + HalfExtent) / VoxelSize
		) + 1,
		0,
		CellsPerAxis
	);

	const int32 MinZ = FMath::Clamp(
		FMath::FloorToInt(
			((LocalLocation.Z - LocalRadius) + HalfExtent) / VoxelSize
		) - 1,
		0,
		CellsPerAxis
	);

	const int32 MaxZ = FMath::Clamp(
		FMath::CeilToInt(
			((LocalLocation.Z + LocalRadius) + HalfExtent) / VoxelSize
		) + 1,
		0,
		CellsPerAxis
	);
	
	/*
 * Record resource cells that contain material before the density carve.
 *
 * We only award a resource when the entire cell becomes empty after
 * carving. That prevents one cell from paying out repeatedly.
 */
	const int32 TotalCellCount =
		CellsPerAxis * CellsPerAxis * CellsPerAxis;

	TArray<uint8> PotentialOreCells;
	PotentialOreCells.Init(0, TotalCellCount);

	const int32 MinCellX = FMath::Clamp(
		MinX,
		0,
		CellsPerAxis - 1
	);

	const int32 MaxCellX = FMath::Clamp(
		MaxX - 1,
		0,
		CellsPerAxis - 1
	);

	const int32 MinCellY = FMath::Clamp(
		MinY,
		0,
		CellsPerAxis - 1
	);

	const int32 MaxCellY = FMath::Clamp(
		MaxY - 1,
		0,
		CellsPerAxis - 1
	);

	const int32 MinCellZ = FMath::Clamp(
		MinZ,
		0,
		CellsPerAxis - 1
	);

	const int32 MaxCellZ = FMath::Clamp(
		MaxZ - 1,
		0,
		CellsPerAxis - 1
	);

	for (int32 Z = MinCellZ; Z <= MaxCellZ; ++Z)
	{
		for (int32 Y = MinCellY; Y <= MaxCellY; ++Y)
		{
			for (int32 X = MinCellX; X <= MaxCellX; ++X)
			{
				const int32 CellIndex = GetCellIndex(X, Y, Z);

				if (
					!CellOreTags.IsValidIndex(CellIndex) ||
					!CellOreRemaining.IsValidIndex(CellIndex) ||
					!CellOreTags[CellIndex].IsValid() ||
					CellOreRemaining[CellIndex] <= KINDA_SMALL_NUMBER
				)
				{
					continue;
				}

				if (HasMaterialInCell(X, Y, Z))
				{
					PotentialOreCells[CellIndex] = 1;
				}
			}
		}
	}

	bool bDensityChanged = false;

	for (int32 Z = MinZ; Z <= MaxZ; ++Z)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				const FVector SamplePosition = GetSampleLocalPosition(X, Y, Z);

				/*
				 * Positive density = solid rock.
				 * Negative density = empty space.
				 *
				 * A subtraction sphere uses:
				 *
				 *     distance(sample, brush centre) - brush radius
				 *
				 * Taking the minimum makes every point inside the brush
				 * become empty, while preserving untouched asteroid material.
				 */
				const float BrushDensity =
					FVector::Distance(SamplePosition, LocalLocation) -
					LocalRadius;

				float& ExistingDensity =
					DensitySamples[
						GetDensityIndex(X, Y, Z, SampleCount)
					];

				if (BrushDensity < ExistingDensity)
				{
					ExistingDensity = BrushDensity;
					bDensityChanged = true;
				}
			}
		}
	}

	if (!bDensityChanged)
	{
		return false;
	}
	
	const TMap<FGameplayTag, float> VaporisedYieldByOreTag =
	CollectVaporisedOreYields(PotentialOreCells);

	BuildMiningYields(VaporisedYieldByOreTag, OutYields);

	const bool bRebuilt = RebuildMeshFromDensity();

	if (bRebuilt)
	{
		ScheduleDetachedFragmentCleanup();

		/*
		 * Report the combined yields from cells depleted by this single carve.
		 * We intentionally do not redraw ore debug markers here: doing so every
		 * mining pulse can create hundreds of transient debug primitives.
		 */
		LogVaporisedOreYields(VaporisedYieldByOreTag);
		
		UE_LOG(
			LogVoraxiaVoxel,
			Log,
			TEXT(
				"Voxel sphere carved: %s | Local Centre=%s | Radius=%.1f"
			),
			*GetName(),
			*LocalLocation.ToString(),
			LocalRadius
		);
	}

	return bRebuilt;
}

void AVoraxiaVoxelSphereActor::CarveDebugSphere()
{
	CarveSphereAtLocalPosition(
		DebugCarveLocalPosition,
		DebugCarveRadius
	);
}

void AVoraxiaVoxelSphereActor::CleanupDetachedFragmentsNow()
{
	CleanupDetachedFragments();
}

void AVoraxiaVoxelSphereActor::ScheduleDetachedFragmentCleanup()
{
	if (!bCleanupDetachedFragments)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();

	TimerManager.ClearTimer(DetachedFragmentCleanupTimerHandle);

	if (DetachedFragmentCleanupDelay <= KINDA_SMALL_NUMBER)
	{
		CleanupDetachedFragments();
		return;
	}

	TimerManager.SetTimer(
		DetachedFragmentCleanupTimerHandle,
		this,
		&AVoraxiaVoxelSphereActor::CleanupDetachedFragments,
		DetachedFragmentCleanupDelay,
		false
	);
}

void AVoraxiaVoxelSphereActor::CleanupDetachedFragments()
{
	if (
		!bCleanupDetachedFragments ||
		!HasValidDensityField()
	)
	{
		return;
	}

	const int32 RemovedCellCount =
		RemoveSmallDetachedSolidIslands();

	if (RemovedCellCount <= 0)
	{
		return;
	}

	const bool bRebuilt = RebuildMeshFromDensity();

	if (bRebuilt)
	{
		UE_LOG(
			LogVoraxiaVoxel,
			Log,
			TEXT(
				"Removed detached voxel debris: %s | Cells Removed=%d"
			),
			*GetName(),
			RemovedCellCount
		);
	}
}

int32 AVoraxiaVoxelSphereActor::RemoveSmallDetachedSolidIslands()
{
	const int32 CellCount = CellsPerAxis;

	if (CellCount <= 0)
	{
		return 0;
	}

	const int32 TotalCellCount =
		CellCount * CellCount * CellCount;

	TArray<uint8> SolidCells;
	SolidCells.Init(0, TotalCellCount);

	for (int32 Z = 0; Z < CellCount; ++Z)
	{
		for (int32 Y = 0; Y < CellCount; ++Y)
		{
			for (int32 X = 0; X < CellCount; ++X)
			{
				const int32 CellIndex = GetCellIndex(X, Y, Z);

				SolidCells[CellIndex] = IsCellSolid(X, Y, Z)
					? 1
					: 0;
			}
		}
	}

	TArray<uint8> VisitedCells;
	VisitedCells.Init(0, TotalCellCount);

	TArray<TArray<int32>> SolidIslands;

	const FIntVector NeighbourOffsets[] =
	{
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0),
		FIntVector(0, 0, 1),
		FIntVector(0, 0, -1)
	};

	for (int32 Z = 0; Z < CellCount; ++Z)
	{
		for (int32 Y = 0; Y < CellCount; ++Y)
		{
			for (int32 X = 0; X < CellCount; ++X)
			{
				const int32 StartIndex = GetCellIndex(X, Y, Z);

				if (
					!SolidCells[StartIndex] ||
					VisitedCells[StartIndex]
				)
				{
					continue;
				}

				TArray<int32> IslandCells;
				TArray<int32> SearchQueue;

				VisitedCells[StartIndex] = 1;
				SearchQueue.Add(StartIndex);

				for (
					int32 QueueIndex = 0;
					QueueIndex < SearchQueue.Num();
					++QueueIndex
				)
				{
					const int32 CurrentIndex =
						SearchQueue[QueueIndex];

					IslandCells.Add(CurrentIndex);

					const int32 CurrentZ =
						CurrentIndex / (CellCount * CellCount);

					const int32 Remaining =
						CurrentIndex -
						CurrentZ * CellCount * CellCount;

					const int32 CurrentY =
						Remaining / CellCount;

					const int32 CurrentX =
						Remaining - CurrentY * CellCount;

					for (const FIntVector& Offset : NeighbourOffsets)
					{
						const int32 NeighbourX =
							CurrentX + Offset.X;

						const int32 NeighbourY =
							CurrentY + Offset.Y;

						const int32 NeighbourZ =
							CurrentZ + Offset.Z;

						if (
							NeighbourX < 0 ||
							NeighbourX >= CellCount ||
							NeighbourY < 0 ||
							NeighbourY >= CellCount ||
							NeighbourZ < 0 ||
							NeighbourZ >= CellCount
						)
						{
							continue;
						}

						const int32 NeighbourIndex =
							GetCellIndex(
								NeighbourX,
								NeighbourY,
								NeighbourZ
							);

						if (
							!SolidCells[NeighbourIndex] ||
							VisitedCells[NeighbourIndex]
						)
						{
							continue;
						}

						VisitedCells[NeighbourIndex] = 1;
						SearchQueue.Add(NeighbourIndex);
					}
				}

				SolidIslands.Add(MoveTemp(IslandCells));
			}
		}
	}

	int32 SolidCellCount = 0;

	for (const uint8 bSolidCell : SolidCells)
	{
		if (bSolidCell)
		{
			++SolidCellCount;
		}
	}

	UE_LOG(
		LogVoraxiaVoxel,
		Log,
		TEXT(
			"Detached fragment scan: %s | Occupied Cells=%d | Islands=%d"
		),
		*GetName(),
		SolidCellCount,
		SolidIslands.Num()
	);
	
	if (SolidIslands.Num() <= 1)
	{
		return 0;
	}

	int32 LargestIslandIndex = 0;

	for (int32 IslandIndex = 1;
		IslandIndex < SolidIslands.Num();
		++IslandIndex)
	{
		if (
			SolidIslands[IslandIndex].Num() >
			SolidIslands[LargestIslandIndex].Num()
		)
		{
			LargestIslandIndex = IslandIndex;
		}
	}

	const int32 SafeMaxFragmentCellCount = FMath::Max(
		MaxDetachedFragmentCellCount,
		1
	);

	if (
		bDrawAllDetachedFragmentIslands &&
		SolidIslands.IsValidIndex(LargestIslandIndex)
	)
	{
		DrawDetachedFragmentCandidateBox(
			SolidIslands[LargestIslandIndex],
			FColor::Cyan
		);
	}

	TArray<uint8> CellsToRemove;
	CellsToRemove.Init(0, TotalCellCount);

	int32 RemovedCellCount = 0;

	for (
		int32 IslandIndex = 0;
		IslandIndex < SolidIslands.Num();
		++IslandIndex
	)
	{
		if (IslandIndex == LargestIslandIndex)
		{
			continue;
		}

		const TArray<int32>& IslandCells =
			SolidIslands[IslandIndex];

		const bool bEligibleForRemoval =
			IslandCells.Num() <= SafeMaxFragmentCellCount;

		if (bDrawAllDetachedFragmentIslands)
		{
			const FColor IslandColour = bEligibleForRemoval
				? FColor(255, 145, 0)
				: FColor(220, 0, 255);

			DrawDetachedFragmentCandidateBox(
				IslandCells,
				IslandColour
			);
		}

		UE_LOG(
			LogVoraxiaVoxel,
			Log,
			TEXT(
				"Detached fragment scan: %s | Island=%d | Cells=%d | "
				"Eligible=%s"
			),
			*GetName(),
			IslandIndex,
			IslandCells.Num(),
			bEligibleForRemoval ? TEXT("Yes") : TEXT("No")
		);

		if (
			!bEligibleForRemoval ||
			bDebugOnlyDetachedFragmentCandidates
		)
		{
			continue;
		}

		for (const int32 CellIndex : IslandCells)
		{
			CellsToRemove[CellIndex] = 1;
			++RemovedCellCount;
		}
	}

	if (RemovedCellCount <= 0)
	{
		return 0;
	}

	const int32 SampleCount = CellsPerAxis + 1;
	const int32 TotalSampleCount =
		SampleCount * SampleCount * SampleCount;

	/*
	 * Protect samples that still belong to retained solid cells.
	 * This stops cleanup of a nearby debris island from nibbling into the
	 * main asteroid through an edge or corner sample.
	 */
	TArray<uint8> ProtectedSamples;
	ProtectedSamples.Init(0, TotalSampleCount);

	for (int32 Z = 0; Z < CellCount; ++Z)
	{
		for (int32 Y = 0; Y < CellCount; ++Y)
		{
			for (int32 X = 0; X < CellCount; ++X)
			{
				const int32 CellIndex = GetCellIndex(X, Y, Z);

				if (
					!SolidCells[CellIndex] ||
					CellsToRemove[CellIndex]
				)
				{
					continue;
				}

				for (int32 CornerZ = 0; CornerZ <= 1; ++CornerZ)
				{
					for (int32 CornerY = 0; CornerY <= 1; ++CornerY)
					{
						for (
							int32 CornerX = 0;
							CornerX <= 1;
							++CornerX
						)
						{
							const int32 SampleIndex =
								GetDensityIndex(
									X + CornerX,
									Y + CornerY,
									Z + CornerZ,
									SampleCount
								);

							ProtectedSamples[SampleIndex] = 1;
						}
					}
				}
			}
		}
	}

	const float EmptyDensity = -FMath::Max(VoxelSize, 1.0f);

	for (int32 Z = 0; Z < CellCount; ++Z)
	{
		for (int32 Y = 0; Y < CellCount; ++Y)
		{
			for (int32 X = 0; X < CellCount; ++X)
			{
				const int32 CellIndex = GetCellIndex(X, Y, Z);

				if (!CellsToRemove[CellIndex])
				{
					continue;
				}

				for (int32 CornerZ = 0; CornerZ <= 1; ++CornerZ)
				{
					for (int32 CornerY = 0; CornerY <= 1; ++CornerY)
					{
						for (
							int32 CornerX = 0;
							CornerX <= 1;
							++CornerX
						)
						{
							const int32 SampleIndex =
								GetDensityIndex(
									X + CornerX,
									Y + CornerY,
									Z + CornerZ,
									SampleCount
								);

							if (!ProtectedSamples[SampleIndex])
							{
								DensitySamples[SampleIndex] =
									EmptyDensity;
							}
						}
					}
				}
			}
		}
	}

	return RemovedCellCount;
}

bool AVoraxiaVoxelSphereActor::IsCellSolid(
	const int32 X,
	const int32 Y,
	const int32 Z
) const
{
	const int32 SampleCount = CellsPerAxis + 1;

	float MaximumDensity = -TNumericLimits<float>::Max();

	for (int32 CornerZ = 0; CornerZ <= 1; ++CornerZ)
	{
		for (int32 CornerY = 0; CornerY <= 1; ++CornerY)
		{
			for (int32 CornerX = 0; CornerX <= 1; ++CornerX)
			{
				const float SampleDensity =
					DensitySamples[
						GetDensityIndex(
							X + CornerX,
							Y + CornerY,
							Z + CornerZ,
							SampleCount
						)
					];

				MaximumDensity = FMath::Max(
					MaximumDensity,
					SampleDensity
				);
			}
		}
	}

	/*
	 * A Marching Cubes cell can produce visible geometry even when most of
	 * its corners are empty. Using the maximum density includes thin shards
	 * and surface-only fragments in the connectivity scan.
	 *
	 * Keep the threshold at zero during this debug pass so every visible
	 * material-bearing cell is eligible for island detection.
	 */
	return MaximumDensity > DetachedFragmentConnectivityErosion;
}

int32 AVoraxiaVoxelSphereActor::GetCellIndex(
	const int32 X,
	const int32 Y,
	const int32 Z
) const
{
	return X + CellsPerAxis * (Y + CellsPerAxis * Z);
}

void AVoraxiaVoxelSphereActor::GenerateSphereDensity()
{
	const int32 SampleCount = CellsPerAxis + 1;

	DensitySamples.SetNumUninitialized(
		SampleCount * SampleCount * SampleCount
	);

	for (int32 Z = 0; Z < SampleCount; ++Z)
	{
		for (int32 Y = 0; Y < SampleCount; ++Y)
		{
			for (int32 X = 0; X < SampleCount; ++X)
			{
				const FVector LocalPosition =
					GetSampleLocalPosition(X, Y, Z);

				/*
				 * Positive = solid voxel material.
				 * Negative = empty space.
				 */
				DensitySamples[
					GetDensityIndex(X, Y, Z, SampleCount)
				] = SphereRadius - LocalPosition.Length();
			}
		}
	}
}

bool AVoraxiaVoxelSphereActor::RebuildMeshFromDensity()
{
	TMap<FGameplayTag, FVoraxiaVoxelMeshData> MeshSections;

	const bool bBuiltMesh =
		FVoraxiaMarchingCubes::BuildIsoSurfaceByMaterial(
			DensitySamples,
			CellsPerAxis,
			VoxelSize,
			CellOreTags,
			BaseMaterialTag,
			MeshSections
		);

	if (!bBuiltMesh)
	{
		VoxelMesh->ClearAllMeshSections();

		UE_LOG(
			LogVoraxiaVoxel,
			Warning,
			TEXT("Voxel sphere remesh produced no geometry: %s"),
			*GetName()
		);

		return false;
	}

	/*
	 * Each ore receives its own procedural mesh section and surface material.
	 * A section contains only triangles generated by cells carrying that ore tag.
	 */
	VoxelMesh->ClearAllMeshSections();

	TArray<FGameplayTag> SectionTags;
	MeshSections.GetKeys(SectionTags);

	SectionTags.Sort(
		[this](
			const FGameplayTag& LeftTag,
			const FGameplayTag& RightTag
		)
		{
			const bool bLeftIsBase =
				LeftTag.MatchesTagExact(BaseMaterialTag);

			const bool bRightIsBase =
				RightTag.MatchesTagExact(BaseMaterialTag);

			if (bLeftIsBase != bRightIsBase)
			{
				return bLeftIsBase;
			}

			return LeftTag.ToString() < RightTag.ToString();
		}
	);

	int32 MeshSectionIndex = 0;
	int32 TotalVertexCount = 0;
	int32 TotalTriangleCount = 0;

	for (const FGameplayTag& MaterialTag : SectionTags)
	{
		FVoraxiaVoxelMeshData* MeshData =
			MeshSections.Find(MaterialTag);

		if (
			!MeshData ||
			MeshData->Vertices.IsEmpty() ||
			MeshData->Triangles.IsEmpty()
		)
		{
			continue;
		}

		VoxelMesh->CreateMeshSection_LinearColor(
			MeshSectionIndex,
			MeshData->Vertices,
			MeshData->Triangles,
			MeshData->Normals,
			MeshData->UV0,
			MeshData->VertexColors,
			MeshData->Tangents,
			bCreateCollision
		);

		UMaterialInterface* SectionMaterial = VoxelMaterial;

		if (IsValid(OreRegistry))
		{
			const UVoraxiaOreDefinition* OreDefinition =
				OreRegistry->FindOreDefinition(MaterialTag);

			if (
				OreDefinition &&
				IsValid(OreDefinition->AsteroidSurfaceMaterial)
			)
			{
				SectionMaterial =
					OreDefinition->AsteroidSurfaceMaterial;
			}
		}

		if (SectionMaterial)
		{
			VoxelMesh->SetMaterial(
				MeshSectionIndex,
				SectionMaterial
			);
		}

		TotalVertexCount += MeshData->Vertices.Num();
		TotalTriangleCount += MeshData->Triangles.Num() / 3;

		++MeshSectionIndex;
	}

	VoxelMesh->SetCollisionEnabled(
		bCreateCollision
			? ECollisionEnabled::QueryAndPhysics
			: ECollisionEnabled::NoCollision
	);

	VoxelMesh->SetSimulatePhysics(false);
	VoxelMesh->SetEnableGravity(false);

	UE_LOG(
		LogVoraxiaVoxel,
		Log,
		TEXT(
			"Voxel sphere remeshed by ore: %s | Sections=%d | "
			"Vertices=%d | Triangles=%d"
		),
		*GetName(),
		MeshSectionIndex,
		TotalVertexCount,
		TotalTriangleCount
	);

	return MeshSectionIndex > 0;
}

bool AVoraxiaVoxelSphereActor::HasValidDensityField() const
{
	const int32 SampleCount = CellsPerAxis + 1;

	return DensitySamples.Num() ==
		SampleCount * SampleCount * SampleCount;
}

FVector AVoraxiaVoxelSphereActor::GetSampleLocalPosition(
	const int32 X,
	const int32 Y,
	const int32 Z
) const
{
	const float HalfExtent =
		static_cast<float>(CellsPerAxis) * VoxelSize * 0.5f;

	return FVector(
		static_cast<float>(X) * VoxelSize - HalfExtent,
		static_cast<float>(Y) * VoxelSize - HalfExtent,
		static_cast<float>(Z) * VoxelSize - HalfExtent
	);
}

int32 AVoraxiaVoxelSphereActor::GetDensityIndex(
	const int32 X,
	const int32 Y,
	const int32 Z,
	const int32 SampleCount
) const
{
	return X + SampleCount * (Y + SampleCount * Z);
}

void AVoraxiaVoxelSphereActor::DrawDetachedFragmentCandidateBox(
	const TArray<int32>& IslandCells,
	const FColor& Colour
) const
{
	if (
		!bDrawDetachedFragmentCandidates ||
		IslandCells.IsEmpty()
	)
	{
		return;
	}

	const UWorld* World = GetWorld();

	if (!World || !VoxelMesh)
	{
		return;
	}

	int32 MinX = CellsPerAxis;
	int32 MinY = CellsPerAxis;
	int32 MinZ = CellsPerAxis;

	int32 MaxX = 0;
	int32 MaxY = 0;
	int32 MaxZ = 0;

	for (const int32 CellIndex : IslandCells)
	{
		const int32 CellZ =
			CellIndex / (CellsPerAxis * CellsPerAxis);

		const int32 Remaining =
			CellIndex -
			CellZ * CellsPerAxis * CellsPerAxis;

		const int32 CellY = Remaining / CellsPerAxis;
		const int32 CellX = Remaining - CellY * CellsPerAxis;

		MinX = FMath::Min(MinX, CellX);
		MinY = FMath::Min(MinY, CellY);
		MinZ = FMath::Min(MinZ, CellZ);

		MaxX = FMath::Max(MaxX, CellX);
		MaxY = FMath::Max(MaxY, CellY);
		MaxZ = FMath::Max(MaxZ, CellZ);
	}

	/*
	 * Cell coordinates become local-space bounds.
	 * Max uses +1 because a cell occupies the interval from its minimum
	 * sample corner to its next sample corner.
	 */
	const FVector LocalMin =
		GetSampleLocalPosition(MinX, MinY, MinZ);

	const FVector LocalMax =
		GetSampleLocalPosition(MaxX + 1, MaxY + 1, MaxZ + 1);

	const FVector LocalCentre = (LocalMin + LocalMax) * 0.5f;
	const FVector LocalExtent =
		(LocalMax - LocalMin) * 0.5f +
		FVector(VoxelSize * 0.08f);

	const FTransform MeshTransform =
		VoxelMesh->GetComponentTransform();

	const FVector WorldCentre =
		MeshTransform.TransformPosition(LocalCentre);

	const FVector WorldScale =
		MeshTransform.GetScale3D().GetAbs();

	const FVector WorldExtent(
		LocalExtent.X * WorldScale.X,
		LocalExtent.Y * WorldScale.Y,
		LocalExtent.Z * WorldScale.Z
	);

	DrawDebugBox(
		World,
		WorldCentre,
		WorldExtent,
		MeshTransform.GetRotation(),
		Colour,
		false,
		FMath::Max(DetachedFragmentDebugLifetime, 0.1f),
		0,
		2.5f
	);
}

void AVoraxiaVoxelSphereActor::GenerateOreData()
{
	const int32 TotalCellCount =
		CellsPerAxis * CellsPerAxis * CellsPerAxis;

	CellOreTags.SetNum(TotalCellCount);
	CellOreRemaining.Init(0.0f, TotalCellCount);

	if (!IsValid(OreRegistry))
	{
		UE_LOG(
			LogVoraxiaVoxel,
			Warning,
			TEXT(
				"Voxel sphere '%s' has no OreRegistry assigned. "
				"Ore deposits were not generated."
			),
			*GetName()
		);

		return;
	}

	if (!BaseMaterialTag.IsValid())
	{
		UE_LOG(
			LogVoraxiaVoxel,
			Warning,
			TEXT(
				"Voxel sphere '%s' has no BaseMaterialTag assigned. "
				"Ore deposits were not generated."
			),
			*GetName()
		);

		return;
	}

	const UVoraxiaOreDefinition* BaseOreDefinition =
		OreRegistry->FindOreDefinition(BaseMaterialTag);

	if (!BaseOreDefinition)
	{
		UE_LOG(
			LogVoraxiaVoxel,
			Warning,
			TEXT(
				"Voxel sphere '%s' could not resolve BaseMaterialTag '%s'."
			),
			*GetName(),
			*BaseMaterialTag.ToString()
		);

		return;
	}

	/*
	 * Initialise all currently solid cells as the base material, normally Rock.
	 */
	for (int32 Z = 0; Z < CellsPerAxis; ++Z)
	{
		for (int32 Y = 0; Y < CellsPerAxis; ++Y)
		{
			for (int32 X = 0; X < CellsPerAxis; ++X)
			{
				if (!HasMaterialInCell(X, Y, Z))
				{
					continue;
				}

				const int32 CellIndex = GetCellIndex(X, Y, Z);

				CellOreTags[CellIndex] = BaseMaterialTag;
				CellOreRemaining[CellIndex] =
					BaseOreDefinition->YieldPerVoxelCell;
			}
		}
	}

	/*
	 * Select up to three valid non-base ore types.
	 */
	TArray<FGameplayTag> SelectedOreTags;
	TSet<FGameplayTag> SeenOreTags;

	const int32 SafeMaxOreTypes = FMath::Clamp(
		MaxOreTypesPerAsteroid,
		1,
		3
	);

	for (const FGameplayTag OreTag : TestOreTags)
	{
		if (
			!OreTag.IsValid() ||
			OreTag == BaseMaterialTag ||
			SeenOreTags.Contains(OreTag)
		)
		{
			continue;
		}

		if (!OreRegistry->FindOreDefinition(OreTag))
		{
			UE_LOG(
				LogVoraxiaVoxel,
				Warning,
				TEXT(
					"Voxel sphere '%s' skipped unresolved ore tag '%s'."
				),
				*GetName(),
				*OreTag.ToString()
			);

			continue;
		}

		SeenOreTags.Add(OreTag);
		SelectedOreTags.Add(OreTag);

		if (SelectedOreTags.Num() >= SafeMaxOreTypes)
		{
			break;
		}
	}

	FRandomStream RandomStream(OreGenerationSeed);

	for (int32 OreIndex = 0; OreIndex < SelectedOreTags.Num(); ++OreIndex)
	{
		const FGameplayTag OreTag = SelectedOreTags[OreIndex];

		for (
			int32 DepositIndex = 0;
			DepositIndex < FMath::Max(OreDepositsPerType, 1);
			++DepositIndex
		)
		{
			/*
			 * Alternate surface-biased and internal deposits for easy testing.
			 *
			 * Ore definitions that cannot appear on the surface are handled
			 * inside GenerateOreDeposit.
			 */
			const bool bPreferSurface =
				((OreIndex + DepositIndex) % 2) == 0;

			GenerateOreDeposit(
				OreTag,
				RandomStream,
				bPreferSurface
			);
		}
	}

	UE_LOG(
		LogVoraxiaVoxel,
		Log,
		TEXT(
			"Voxel sphere ore data generated: %s | Base=%s | "
			"Ore Types=%d | Seed=%d"
		),
		*GetName(),
		*BaseMaterialTag.ToString(),
		SelectedOreTags.Num(),
		OreGenerationSeed
	);
}

void AVoraxiaVoxelSphereActor::GenerateOreDeposit(
	const FGameplayTag OreTag,
	FRandomStream& RandomStream,
	const bool bPreferSurface
)
{
	if (!IsValid(OreRegistry))
	{
		return;
	}

	const UVoraxiaOreDefinition* OreDefinition =
		OreRegistry->FindOreDefinition(OreTag);

	if (!OreDefinition)
	{
		return;
	}

	const float SafeMinRadius = FMath::Max(
		MinOreDepositRadius,
		VoxelSize
	);

	const float SafeMaxRadius = FMath::Max(
		MaxOreDepositRadius,
		SafeMinRadius
	);

	const float DepositRadius = RandomStream.FRandRange(
		SafeMinRadius,
		SafeMaxRadius
	);

	const bool bCanUseSurfacePlacement =
		bPreferSurface &&
		OreDefinition->bCanAppearOnSurface;

	FVector LocalCentre = FVector::ZeroVector;

	bool bFoundValidCentre = false;

	for (int32 Attempt = 0; Attempt < 32; ++Attempt)
	{
		FVector Direction = RandomStream.VRand();

		if (Direction.IsNearlyZero())
		{
			Direction = FVector::UpVector;
		}

		const float SafeInteriorRadius = FMath::Max(
			SphereRadius - DepositRadius - VoxelSize,
			0.0f
		);

		const float CentreDistance = bCanUseSurfacePlacement
			? FMath::Max(
				SafeInteriorRadius,
				SphereRadius - DepositRadius * 0.65f
			)
			: RandomStream.FRandRange(
				0.0f,
				SafeInteriorRadius * 0.60f
			);

		const FVector CandidateCentre =
			Direction * CentreDistance;

		const int32 TestX = FMath::Clamp(
			FMath::FloorToInt(
				(CandidateCentre.X +
					static_cast<float>(CellsPerAxis) * VoxelSize * 0.5f) /
				VoxelSize
			),
			0,
			CellsPerAxis - 1
		);

		const int32 TestY = FMath::Clamp(
			FMath::FloorToInt(
				(CandidateCentre.Y +
					static_cast<float>(CellsPerAxis) * VoxelSize * 0.5f) /
				VoxelSize
			),
			0,
			CellsPerAxis - 1
		);

		const int32 TestZ = FMath::Clamp(
			FMath::FloorToInt(
				(CandidateCentre.Z +
					static_cast<float>(CellsPerAxis) * VoxelSize * 0.5f) /
				VoxelSize
			),
			0,
			CellsPerAxis - 1
		);

		if (!HasMaterialInCell(TestX, TestY, TestZ))
		{
			continue;
		}

		LocalCentre = CandidateCentre;
		bFoundValidCentre = true;
		break;
	}

	if (!bFoundValidCentre)
	{
		UE_LOG(
			LogVoraxiaVoxel,
			Warning,
			TEXT(
				"Voxel sphere '%s' could not place ore deposit '%s'."
			),
			*GetName(),
			*OreTag.ToString()
		);

		return;
	}

	int32 AssignedCellCount = 0;

	for (int32 Z = 0; Z < CellsPerAxis; ++Z)
	{
		for (int32 Y = 0; Y < CellsPerAxis; ++Y)
		{
			for (int32 X = 0; X < CellsPerAxis; ++X)
			{
				if (!HasMaterialInCell(X, Y, Z))
				{
					continue;
				}

				const FVector CellCentre =
					GetCellLocalCentre(X, Y, Z);

				if (
					FVector::Distance(
						CellCentre,
						LocalCentre
					) > DepositRadius
				)
				{
					continue;
				}

				const int32 CellIndex = GetCellIndex(X, Y, Z);

				CellOreTags[CellIndex] = OreTag;
				CellOreRemaining[CellIndex] =
					OreDefinition->YieldPerVoxelCell;

				++AssignedCellCount;
			}
		}
	}

	UE_LOG(
		LogVoraxiaVoxel,
		Log,
		TEXT(
			"Ore deposit generated: %s | Ore=%s | Cells=%d | "
			"Radius=%.1f | SurfaceBiased=%s"
		),
		*GetName(),
		*OreTag.ToString(),
		AssignedCellCount,
		DepositRadius,
		bCanUseSurfacePlacement ? TEXT("Yes") : TEXT("No")
	);
}

bool AVoraxiaVoxelSphereActor::IsCellDepletedForYield(
	const int32 X,
	const int32 Y,
	const int32 Z
) const
{
	const int32 SampleCount = CellsPerAxis + 1;

	float DensityTotal = 0.0f;

	for (int32 CornerZ = 0; CornerZ <= 1; ++CornerZ)
	{
		for (int32 CornerY = 0; CornerY <= 1; ++CornerY)
		{
			for (int32 CornerX = 0; CornerX <= 1; ++CornerX)
			{
				DensityTotal += DensitySamples[
					GetDensityIndex(
						X + CornerX,
						Y + CornerY,
						Z + CornerZ,
						SampleCount
					)
				];
			}
		}
	}

	const float AverageDensity = DensityTotal / 8.0f;

	/*
	 * Yield when the cell's average density has crossed into empty space.
	 *
	 * This is more reliable than requiring all eight shared corners to be
	 * negative, while still ensuring a cell is materially depleted before
	 * it pays out.
	 */
	return AverageDensity <= 0.0f;
}

bool AVoraxiaVoxelSphereActor::HasMaterialInCell(
	const int32 X,
	const int32 Y,
	const int32 Z
) const
{
	const int32 SampleCount = CellsPerAxis + 1;

	for (int32 CornerZ = 0; CornerZ <= 1; ++CornerZ)
	{
		for (int32 CornerY = 0; CornerY <= 1; ++CornerY)
		{
			for (int32 CornerX = 0; CornerX <= 1; ++CornerX)
			{
				const float Density =
					DensitySamples[
						GetDensityIndex(
							X + CornerX,
							Y + CornerY,
							Z + CornerZ,
							SampleCount
						)
					];

				/*
				 * This intentionally uses zero, not
				 * DetachedFragmentConnectivityErosion.
				 *
				 * It answers whether the density field contains actual
				 * material, not whether a cell should count as connected
				 * for fragment cleanup.
				 */
				if (Density > 0.0f)
				{
					return true;
				}
			}
		}
	}

	return false;
}

FVector AVoraxiaVoxelSphereActor::GetCellLocalCentre(
	const int32 X,
	const int32 Y,
	const int32 Z
) const
{
	return (
		GetSampleLocalPosition(X, Y, Z) +
		GetSampleLocalPosition(X + 1, Y + 1, Z + 1)
	) * 0.5f;
}

TMap<FGameplayTag, float>
AVoraxiaVoxelSphereActor::CollectVaporisedOreYields(
	const TArray<uint8>& PotentialOreCells
)
{
	TMap<FGameplayTag, float> YieldByOreTag;

	const int32 TotalCellCount =
		CellsPerAxis * CellsPerAxis * CellsPerAxis;

	if (
		PotentialOreCells.Num() != TotalCellCount ||
		CellOreTags.Num() != TotalCellCount ||
		CellOreRemaining.Num() != TotalCellCount
	)
	{
		return YieldByOreTag;
	}

	for (int32 Z = 0; Z < CellsPerAxis; ++Z)
	{
		for (int32 Y = 0; Y < CellsPerAxis; ++Y)
		{
			for (int32 X = 0; X < CellsPerAxis; ++X)
			{
				const int32 CellIndex = GetCellIndex(X, Y, Z);

				if (
					!PotentialOreCells[CellIndex] ||
					!CellOreTags[CellIndex].IsValid() ||
					CellOreRemaining[CellIndex] <= KINDA_SMALL_NUMBER
				)
				{
					continue;
				}

				/*
				 * The cell contained material before this carve. Only award
				 * it when it no longer contains any positive density.
				 */
				if (!IsCellDepletedForYield(X, Y, Z))
				{
					continue;
				}
				
				UE_LOG(
				LogVoraxiaVoxel,
				Log,
				TEXT(
				"Ore cell depleted: %s | Tag=%s | Yield=%.2f"
				),
				*GetName(),
				*CellOreTags[CellIndex].ToString(),
				CellOreRemaining[CellIndex]
);

				YieldByOreTag.FindOrAdd(
					CellOreTags[CellIndex]
				) += CellOreRemaining[CellIndex];

				/*
				 * Clear it permanently so this voxel can never pay twice.
				 */
				CellOreRemaining[CellIndex] = 0.0f;
				CellOreTags[CellIndex] = FGameplayTag();
			}
		}
	}

	return YieldByOreTag;
}

void AVoraxiaVoxelSphereActor::BuildMiningYields(
	const TMap<FGameplayTag, float>& YieldByOreTag,
	TArray<FVoraxiaMiningYield>& OutYields
) const
{
	OutYields.Reset();

	for (const TPair<FGameplayTag, float>& Pair : YieldByOreTag)
	{

		if (
			!Pair.Key.IsValid() ||
			Pair.Value <= KINDA_SMALL_NUMBER
		)
		{
			continue;
		}

		FVoraxiaMiningYield& MiningYield = OutYields.AddDefaulted_GetRef();
		MiningYield.ResourceTag = Pair.Key;
		MiningYield.Amount = Pair.Value;
	}
}

void AVoraxiaVoxelSphereActor::LogVaporisedOreYields(
	const TMap<FGameplayTag, float>& YieldByOreTag
) const
{
	for (const TPair<FGameplayTag, float>& Pair : YieldByOreTag)
	{
		const UVoraxiaOreDefinition* OreDefinition =
			IsValid(OreRegistry)
				? OreRegistry->FindOreDefinition(Pair.Key)
				: nullptr;

		const FString OreName = OreDefinition
			? OreDefinition->DisplayName.ToString()
			: Pair.Key.ToString();

		UE_LOG(
			LogVoraxiaVoxel,
			Log,
			TEXT(
				"Raptor vaporised resource: %s | Tag=%s | Yield=%.2f"
			),
			*OreName,
			*Pair.Key.ToString(),
			Pair.Value
		);
	}
}

void AVoraxiaVoxelSphereActor::DrawOreDebugCells() const
{
	if (
		!bDrawOreCells ||
		!IsValid(OreRegistry) ||
		!VoxelMesh
	)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const FTransform MeshTransform =
		VoxelMesh->GetComponentTransform();

	const FVector WorldScale =
		MeshTransform.GetScale3D().GetAbs();

	/*
	 * Debug-only safety cap.
	 *
	 * A real deposit can span hundreds of voxel cells. Drawing a sphere and
	 * a line for every cell turns the debug overlay into a frame-rate sink.
	 * We instead sample a maximum of 48 projected markers.
	 */
	constexpr int32 MaxDebugMarkers = 48;

	int32 VisibleOreCellCount = 0;

	for (int32 Z = 0; Z < CellsPerAxis; ++Z)
	{
		for (int32 Y = 0; Y < CellsPerAxis; ++Y)
		{
			for (int32 X = 0; X < CellsPerAxis; ++X)
			{
				const int32 CellIndex = GetCellIndex(X, Y, Z);

				if (
					!CellOreTags.IsValidIndex(CellIndex) ||
					!CellOreRemaining.IsValidIndex(CellIndex) ||
					!CellOreTags[CellIndex].IsValid() ||
					CellOreRemaining[CellIndex] <= KINDA_SMALL_NUMBER
				)
				{
					continue;
				}

				if (
					bDrawOnlyNonBaseOreCells &&
					CellOreTags[CellIndex] == BaseMaterialTag
				)
				{
					continue;
				}

				++VisibleOreCellCount;
			}
		}
	}

	if (VisibleOreCellCount <= 0)
	{
		return;
	}

	const int32 DebugStride = FMath::Max(
		1,
		FMath::CeilToInt(
			static_cast<float>(VisibleOreCellCount) /
			static_cast<float>(MaxDebugMarkers)
		)
	);

	const float SurfaceDisplayRadius =
		SphereRadius + VoxelSize * 0.75f;

	const float DebugLifetime =
		FMath::Clamp(OreDebugLifetime, 0.1f, 10.0f);

	int32 SeenOreCellCount = 0;
	int32 DrawnMarkerCount = 0;

	for (int32 Z = 0; Z < CellsPerAxis; ++Z)
	{
		for (int32 Y = 0; Y < CellsPerAxis; ++Y)
		{
			for (int32 X = 0; X < CellsPerAxis; ++X)
			{
				const int32 CellIndex = GetCellIndex(X, Y, Z);

				if (
					!CellOreTags.IsValidIndex(CellIndex) ||
					!CellOreRemaining.IsValidIndex(CellIndex) ||
					!CellOreTags[CellIndex].IsValid() ||
					CellOreRemaining[CellIndex] <= KINDA_SMALL_NUMBER
				)
				{
					continue;
				}

				if (
					bDrawOnlyNonBaseOreCells &&
					CellOreTags[CellIndex] == BaseMaterialTag
				)
				{
					continue;
				}

				++SeenOreCellCount;

				if ((SeenOreCellCount % DebugStride) != 0)
				{
					continue;
				}

				const UVoraxiaOreDefinition* OreDefinition =
					OreRegistry->FindOreDefinition(
						CellOreTags[CellIndex]
					);

				if (!OreDefinition)
				{
					continue;
				}

				const FVector LocalCellCentre =
					GetCellLocalCentre(X, Y, Z);

				FVector SurfaceDirection =
					LocalCellCentre.GetSafeNormal();

				if (SurfaceDirection.IsNearlyZero())
				{
					SurfaceDirection = FVector::UpVector;
				}

				const FVector LocalSurfacePosition =
					SurfaceDirection * SurfaceDisplayRadius;

				const FVector WorldSurfacePosition =
					MeshTransform.TransformPosition(
						LocalSurfacePosition
					);

				const float WorldMarkerRadius =
					VoxelSize *
					FMath::Max3(
						WorldScale.X,
						WorldScale.Y,
						WorldScale.Z
					) *
					0.22f;

				DrawDebugSphere(
					World,
					WorldSurfacePosition,
					WorldMarkerRadius,
					8,
					OreDefinition->DebugColour.ToFColor(true),
					false,
					DebugLifetime,
					0,
					2.0f
				);

				++DrawnMarkerCount;

				if (DrawnMarkerCount >= MaxDebugMarkers)
				{
					UE_LOG(
						LogVoraxiaVoxel,
						Verbose,
						TEXT(
							"Ore debug markers capped: %s | "
							"Ore Cells=%d | Markers=%d"
						),
						*GetName(),
						VisibleOreCellCount,
						DrawnMarkerCount
					);

					return;
				}
			}
		}
	}
}
