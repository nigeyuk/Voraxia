// Copyright 2026 Coding Custard Studios.

#include "Asteroids/VoraxiaVoxelSphereActor.h"

#include "Meshing/VoraxiaMarchingCubes.h"

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

void AVoraxiaVoxelSphereActor::BuildVoxelSphere()
{
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