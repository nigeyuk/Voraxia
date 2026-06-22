// Copyright 2026 Coding Custard Studios.

#include "Asteroids/VoraxiaVoxelSphereActor.h"

#include "Meshing/VoraxiaMarchingCubes.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"
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
	if (!Hit.bBlockingHit || WorldBrushRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	return CarveSphereAtWorldLocation(
		Hit.ImpactPoint,
		WorldBrushRadius
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
	RebuildMeshFromDensity();
}

bool AVoraxiaVoxelSphereActor::CarveSphereAtWorldLocation(
	const FVector& WorldLocation,
	const float WorldRadius
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

	return CarveSphereAtLocalPosition(LocalLocation, LocalRadius);
}

bool AVoraxiaVoxelSphereActor::CarveSphereAtLocalPosition(
	const FVector& LocalLocation,
	const float LocalRadius
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

	const bool bRebuilt = RebuildMeshFromDensity();

	if (bRebuilt)
	{
		ScheduleDetachedFragmentCleanup();
		
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
	FVoraxiaVoxelMeshData MeshData;

	const bool bBuiltMesh = FVoraxiaMarchingCubes::BuildIsoSurface(
		DensitySamples,
		CellsPerAxis,
		VoxelSize,
		MeshData
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
	* This actor intentionally owns only one mesh section. Clearing it first
	* ensures removed detached islands cannot leave stale visual geometry or
	* collision behind.
	*/
	VoxelMesh->ClearAllMeshSections();
	
	VoxelMesh->CreateMeshSection_LinearColor(
		0,
		MeshData.Vertices,
		MeshData.Triangles,
		MeshData.Normals,
		MeshData.UV0,
		MeshData.VertexColors,
		MeshData.Tangents,
		bCreateCollision
	);

	VoxelMesh->SetCollisionEnabled(
		bCreateCollision
			? ECollisionEnabled::QueryAndPhysics
			: ECollisionEnabled::NoCollision
	);

	VoxelMesh->SetSimulatePhysics(false);
	VoxelMesh->SetEnableGravity(false);

	if (VoxelMaterial)
	{
		VoxelMesh->SetMaterial(0, VoxelMaterial);
	}

	UE_LOG(
		LogVoraxiaVoxel,
		Log,
		TEXT(
			"Voxel sphere remeshed: %s | Vertices=%d | Triangles=%d"
		),
		*GetName(),
		MeshData.Vertices.Num(),
		MeshData.Triangles.Num() / 3
	);

	return true;
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