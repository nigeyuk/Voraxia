// Copyright 2026 Coding Custard Studios.


#include "Meshing/VoraxiaMarchingCubes.h"

#include "Misc/Base64.h"

namespace VoraxiaVoxel::MarchingCubes
{
	/*
	 * Standard Marching Cubes corner order:
	 *
	 *        7 -------- 6
	 *       /|         /|
	 *      4 -------- 5 |
	 *      | |        | |
	 *      | 3 -------|-2
	 *      |/         |/
	 *      0 -------- 1
	 */
	static constexpr int32 CornerOffsets[8][3] =
	{
		{ 0, 0, 0 },
		{ 1, 0, 0 },
		{ 1, 1, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
		{ 1, 0, 1 },
		{ 1, 1, 1 },
		{ 0, 1, 1 }
	};

	/*
	 * Standard Marching Cubes edge order.
	 */
	static constexpr int32 EdgeCorners[12][2] =
	{
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
		{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
	};

	/*
	 * The classic 256-case Marching Cubes triangle table, encoded as Base64.
	 *
	 * It expands into 256 rows of 16 signed bytes. Each three-byte group
	 * describes the cube edges that form a triangle. -1 ends the row.
	 *
	 * Keeping it encoded prevents this source file becoming a 17,000-character
	 * wall of raw table values while still using the real lookup-table algorithm.
	 */
	static const FString TriangleTableBase64 =
		TEXT("/////////////////////wAIA/////////////////8AAQn/////////////////AQgDCQgB/////////////wECCv////////////////8ACAMBAgr/////")
		TEXT("////////CQIKAAIJ/////////////wIIAwIKCAoJCP////////8DCwL/////////////////AAsCCAsA/////////////wEJAAIDC/////////////8BCwIB")
		TEXT("CQsJCAv/////////AwoBCwoD/////////////wAKAQAICggLCv////////8DCQADCwkLCgn/////////CQgKCggL/////////////wQHCP//////////////")
		TEXT("//8EAwAHAwT/////////////AAEJCAQH/////////////wQBCQQHAQcDAf////////8BAgoIBAf/////////////AwQHAwAEAQIK/////////wkCCgkAAggE")
		TEXT("B/////////8CCgkCCQcCBwMHCQT/////CAQHAwsC/////////////wsEBwsCBAIABP////////8JAAEIBAcCAwv/////////BAcLCQQLCQsCCQIB/////wMK")
		TEXT("AQMLCgcIBP////////8BCwoBBAsBAAQHCwT/////BAcICQALCQsKCwAD/////wQHCwQLCQkLCv////////8JBQT/////////////////CQUEAAgD////////")
		TEXT("/////wAFBAEFAP////////////8IBQQIAwUDAQX/////////AQIKCQUE/////////////wMACAECCgQJBf////////8FAgoFBAIEAAL/////////AgoFAwIF")
		TEXT("AwUEAwQI/////wkFBAIDC/////////////8ACwIACAsECQX/////////AAUEAAEFAgML/////////wIBBQIFCAIICwQIBf////8KAwsKAQMJBQT/////////")
		TEXT("BAkFAAgBCAoBCAsK/////wUEAAUACwULCgsAA/////8FBAgFCAoKCAv/////////CQcIBQcJ/////////////wkDAAkFAwUHA/////////8ABwgAAQcBBQf/")
		TEXT("////////AQUDAwUH/////////////wkHCAkFBwoBAv////////8KAQIJBQAFAwAFBwP/////CAACCAIFCAUHCgUC/////wIKBQIFAwMFB/////////8HCQUH")
		TEXT("CAkDCwL/////////CQUHCQcCCQIAAgcL/////wIDCwABCAEHCAEFB/////8LAgELAQcHAQX/////////CQUICAUHCgEDCgML/////wUHAAUACQcLAAEACgsK")
		TEXT("AP8LCgALAAMKBQAIAAcFBwD/CwoFBwsF/////////////woGBf////////////////8ACAMFCgb/////////////CQABBQoG/////////////wEIAwEJCAUK")
		TEXT("Bv////////8BBgUCBgH/////////////AQYFAQIGAwAI/////////wkGBQkABgACBv////////8FCQgFCAIFAgYDAgj/////AgMLCgYF/////////////wsA")
		TEXT("CAsCAAoGBf////////8AAQkCAwsFCgb/////////BQoGAQkCCQsCCQgL/////wYDCwYFAwUBA/////////8ACAsACwUABQEFCwb/////AwsGAAMGAAYFAAUJ")
		TEXT("/////wYFCQYJCwsJCP////////8FCgYEBwj/////////////BAMABAcDBgUK/////////wEJAAUKBggEB/////////8KBgUBCQcBBwMHCQT/////BgECBgUB")
		TEXT("BAcI/////////wECBQUCBgMABAMEB/////8IBAcJAAUABgUAAgb/////BwMJBwkEAwIJBQkGAgYJ/wMLAgcIBAoGBf////////8FCgYEBwIEAgACBwv/////")
		TEXT("AAEJBAcIAgMLBQoG/////wkCAQkLAgkECwcLBAUKBv8IBAcDCwUDBQEFCwb/////BQELBQsGAQALBwsEAAQL/wAFCQAGBQADBgsGAwgEB/8GBQkGCQsEBwkH")
		TEXT("Cwn/////CgQJBgQK/////////////wQKBgQJCgAIA/////////8KAAEKBgAGBAD/////////CAMBCAEGCAYEBgEK/////wEECQECBAIGBP////////8DAAgB")
		TEXT("AgkCBAkCBgT/////AAIEBAIG/////////////wgDAggCBAQCBv////////8KBAkKBgQLAgP/////////AAgCAggLBAkKBAoG/////wMLAgABBgAGBAYBCv//")
		TEXT("//8GBAEGAQoECAECAQsICwH/CQYECQMGCQEDCwYD/////wgLAQgBAAsGAQkBBAYEAf8DCwYDBgAABgT/////////BgQICwYI/////////////wcKBgcICggJ")
		TEXT("Cv////////8ABwMACgcACQoGBwr/////CgYHAQoHAQcIAQgA/////woGBwoHAQEHA/////////8BAgYBBggBCAkIBgf/////AgYJAgkBBgcJAAkDBwMJ/wcI")
		TEXT("AAcABgYAAv////////8HAwIGBwL/////////////AgMLCgYICggJCAYH/////wIABwIHCwAJBwYHCgkKB/8BCAABBwgBCgcGBwoCAwv/CwIBCwEHCgYBBgcB")
		TEXT("/////wgJBggGBwkBBgsGAwEDBv8ACQELBgf/////////////BwgABwAGAwsACwYA/////wcLBv////////////////8HBgv/////////////////AwAICwcG")
		TEXT("/////////////wABCQsHBv////////////8IAQkIAwELBwb/////////CgECBgsH/////////////wECCgMACAYLB/////////8CCQACCgkGCwf/////////")
		TEXT("BgsHAgoDCggDCgkI/////wcCAwYCB/////////////8HAAgHBgAGAgD/////////AgcGAgMHAAEJ/////////wEGAgEIBgEJCAgHBv////8KBwYKAQcBAwf/")
		TEXT("////////CgcGAQcKAQgHAQAI/////wADBwAHCgAKCQYKB/////8HBgoHCggICgn/////////BggECwgG/////////////wMGCwMABgAEBv////////8IBgsI")
		TEXT("BAYJAAH/////////CQQGCQYDCQMBCwMG/////wYIBAYLCAIKAf////////8BAgoDAAsABgsABAb/////BAsIBAYLAAIJAgoJ/////woJAwoDAgkEAwsDBgQG")
		TEXT("A/8IAgMIBAIEBgL/////////AAQCBAYC/////////////wEJAAIDBAIEBgQDCP////8BCQQBBAICBAb/////////CAEDCAYBCAQGBgoB/////woBAAoABgYA")
		TEXT("BP////////8EBgMEAwgGCgMAAwkKCQP/CgkEBgoE/////////////wQJBQcGC/////////////8ACAMECQULBwb/////////BQABBQQABwYL/////////wsH")
		TEXT("BggDBAMFBAMBBf////8JBQQKAQIHBgv/////////BgsHAQIKAAgDBAkF/////wcGCwUECgQCCgQAAv////8DBAgDBQQDAgUKBQILBwb/BwIDBwYCBQQJ////")
		TEXT("/////wkFBAAIBgAGAgYIB/////8DBgIDBwYBBQAFBAD/////BgIIBggHAgEIBAgFAQUI/wkFBAoBBgEHBgEDB/////8BBgoBBwYBAAcIBwAJBQT/BAAKBAoF")
		TEXT("AAMKBgoHAwcK/wcGCgcKCAUECgQICv////8GCQUGCwkLCAn/////////AwYLAAYDAAUGAAkF/////wALCAAFCwABBQUGC/////8GCwMGAwUFAwH/////////")
		TEXT("AQIKCQULCQsICwUG/////wALAwAGCwAJBgUGCQECCv8LCAULBQYIAAUKBQIAAgX/BgsDBgMFAgoDCgUD/////wUICQUCCAUGAgMIAv////8JBQYJBgAABgL/")
		TEXT("////////AQUIAQgABQYIAwgCBgII/wEFBgIBBv////////////8BAwYBBgoDCAYFBgkICQb/CgEACgAGCQUABQYA/////wADCAUGCv////////////8KBQb/")
		TEXT("////////////////CwUKBwUL/////////////wsFCgsHBQgDAP////////8FCwcFCgsBCQD/////////CgcFCgsHCQgBCAMB/////wsBAgsHAQcFAf//////")
		TEXT("//8ACAMBAgcBBwUHAgv/////CQcFCQIHCQACAgsH/////wcFAgcCCwUJAgMCCAkIAv8CBQoCAwUDBwX/////////CAIACAUCCAcFCgIF/////wkAAQUKAwUD")
		TEXT("BwMKAv////8JCAIJAgEIBwIKAgUHBQL/AQMFAwcF/////////////wAIBwAHAQEHBf////////8JAAMJAwUFAwf/////////CQgHBQkH/////////////wUI")
		TEXT("BAUKCAoLCP////////8FAAQFCwAFCgsLAwD/////AAEJCAQKCAoLCgQF/////woLBAoEBQsDBAkEAQMBBP8CBQECCAUCCwgEBQj/////AAQLAAsDBAULAgsB")
		TEXT("BQEL/wACBQAFCQILBQQFCAsIBf8JBAUCCwP/////////////AgUKAwUCAwQFAwgE/////wUKAgUCBAQCAP////////8DCgIDBQoDCAUEBQgAAQn/BQoCBQIE")
		TEXT("AQkCCQQC/////wgEBQgFAwMFAf////////8ABAUBAAX/////////////CAQFCAUDCQAFAAMF/////wkEBf////////////////8ECwcECQsJCgv/////////")
		TEXT("AAgDBAkHCQsHCQoL/////wEKCwELBAEEAAcEC/////8DAQQDBAgBCgQHBAsKCwT/BAsHCQsECQILCQEC/////wkHBAkLBwkBCwILAQAIA/8LBwQLBAICBAD/")
		TEXT("////////CwcECwQCCAMEAwIE/////wIJCgIHCQIDBwcECf////8JCgcJBwQKAgcIBwACAAf/AwcKAwoCBwQKAQoABAAK/wEKAggHBP////////////8ECQEE")
		TEXT("AQcHAQP/////////BAkBBAEHAAgBCAcB/////wQAAwcEA/////////////8ECAf/////////////////CQoICgsI/////////////wMACQMJCwsJCv//////")
		TEXT("//8AAQoACggICgv/////////AwEKCwMK/////////////wECCwELCQkLCP////////8DAAkDCQsBAgkCCwn/////AAILCAAL/////////////wMCC///////")
		TEXT("//////////8CAwgCCAoKCAn/////////CQoCAAkC/////////////wIDCAIICgABCAEKCP////8BCgL/////////////////AQMICQEI/////////////wAJ")
		TEXT("Af////////////////8AAwj//////////////////////////////////////w==");

	class FTriangleTable final
	{
	public:
		FTriangleTable()
		{
			const bool bDecoded = FBase64::Decode(
				TriangleTableBase64,
				Bytes
			);

			checkf(
				bDecoded && Bytes.Num() == 256 * 16,
				TEXT("Voraxia Marching Cubes triangle table failed to decode.")
			);
		}

		int8 Get(
			const uint8 CaseIndex,
			const int32 EntryIndex
		) const
		{
			return static_cast<int8>(
				Bytes[static_cast<int32>(CaseIndex) * 16 + EntryIndex]
			);
		}

	private:
		TArray<uint8> Bytes;
	};

	static const FTriangleTable& GetTriangleTable()
	{
		static const FTriangleTable Table;
		return Table;
	}

	struct FScalarFieldView final
	{
		const TArray<float>& Samples;
		int32 Cells = 0;
		int32 SampleCount = 0;
		float CellSize = 0.0f;
		float HalfExtent = 0.0f;

		FScalarFieldView(
			const TArray<float>& InSamples,
			const int32 InCells,
			const int32 InSampleCount,
			const float InCellSize,
			const float InHalfExtent
		)
			: Samples(InSamples)
			, Cells(InCells)
			, SampleCount(InSampleCount)
			, CellSize(InCellSize)
			, HalfExtent(InHalfExtent)
		{
		}

		int32 Index(
			const int32 X,
			const int32 Y,
			const int32 Z
		) const
		{
			return X + SampleCount * (Y + SampleCount * Z);
		}

		float Get(
			const int32 X,
			const int32 Y,
			const int32 Z
		) const
		{
			return Samples[Index(X, Y, Z)];
		}

		FVector Position(
			const int32 X,
			const int32 Y,
			const int32 Z
		) const
		{
			return FVector(
				static_cast<float>(X) * CellSize - HalfExtent,
				static_cast<float>(Y) * CellSize - HalfExtent,
				static_cast<float>(Z) * CellSize - HalfExtent
			);
		}

		FVector Gradient(
			const int32 X,
			const int32 Y,
			const int32 Z
		) const
		{
			const int32 PreviousX = FMath::Max(X - 1, 0);
			const int32 NextX = FMath::Min(X + 1, Cells);

			const int32 PreviousY = FMath::Max(Y - 1, 0);
			const int32 NextY = FMath::Min(Y + 1, Cells);

			const int32 PreviousZ = FMath::Max(Z - 1, 0);
			const int32 NextZ = FMath::Min(Z + 1, Cells);

			const float DeltaX = FMath::Max(
				static_cast<float>(NextX - PreviousX) * CellSize,
				KINDA_SMALL_NUMBER
			);

			const float DeltaY = FMath::Max(
				static_cast<float>(NextY - PreviousY) * CellSize,
				KINDA_SMALL_NUMBER
			);

			const float DeltaZ = FMath::Max(
				static_cast<float>(NextZ - PreviousZ) * CellSize,
				KINDA_SMALL_NUMBER
			);

			return FVector(
				(Get(NextX, Y, Z) - Get(PreviousX, Y, Z)) / DeltaX,
				(Get(X, NextY, Z) - Get(X, PreviousY, Z)) / DeltaY,
				(Get(X, Y, NextZ) - Get(X, Y, PreviousZ)) / DeltaZ
			);
		}
	};

	/*
	 * Vertices on grid edges are shared between adjacent cells.
	 *
	 * This avoids the usual beginner-prototype issue where every cube creates
	 * its own duplicate vertices, bloating the mesh and causing visible seams.
	 */
	struct FEdgeVertexCache final
	{
		int32 Cells = 0;
		int32 SampleCount = 0;

		TArray<int32> XEdges;
		TArray<int32> YEdges;
		TArray<int32> ZEdges;

		explicit FEdgeVertexCache(const int32 InCells)
			: Cells(InCells)
			, SampleCount(InCells + 1)
		{
			XEdges.Init(
				INDEX_NONE,
				Cells * SampleCount * SampleCount
			);

			YEdges.Init(
				INDEX_NONE,
				SampleCount * Cells * SampleCount
			);

			ZEdges.Init(
				INDEX_NONE,
				SampleCount * SampleCount * Cells
			);
		}

		int32& Get(
			const int32 CellX,
			const int32 CellY,
			const int32 CellZ,
			const int32 EdgeIndex
		)
		{
			switch (EdgeIndex)
			{
			case 0:
				return XEdges[GetXIndex(CellX, CellY, CellZ)];

			case 1:
				return YEdges[GetYIndex(CellX + 1, CellY, CellZ)];

			case 2:
				return XEdges[GetXIndex(CellX, CellY + 1, CellZ)];

			case 3:
				return YEdges[GetYIndex(CellX, CellY, CellZ)];

			case 4:
				return XEdges[GetXIndex(CellX, CellY, CellZ + 1)];

			case 5:
				return YEdges[GetYIndex(CellX + 1, CellY, CellZ + 1)];

			case 6:
				return XEdges[GetXIndex(CellX, CellY + 1, CellZ + 1)];

			case 7:
				return YEdges[GetYIndex(CellX, CellY, CellZ + 1)];

			case 8:
				return ZEdges[GetZIndex(CellX, CellY, CellZ)];

			case 9:
				return ZEdges[GetZIndex(CellX + 1, CellY, CellZ)];

			case 10:
				return ZEdges[GetZIndex(CellX + 1, CellY + 1, CellZ)];

			default:
				check(EdgeIndex == 11);

				return ZEdges[
					GetZIndex(CellX, CellY + 1, CellZ)
				];
			}
		}

	private:
		int32 GetXIndex(
			const int32 X,
			const int32 Y,
			const int32 Z
		) const
		{
			return X + Cells * (Y + SampleCount * Z);
		}

		int32 GetYIndex(
			const int32 X,
			const int32 Y,
			const int32 Z
		) const
		{
			return X + SampleCount * (Y + Cells * Z);
		}

		int32 GetZIndex(
			const int32 X,
			const int32 Y,
			const int32 Z
		) const
		{
			return X + SampleCount * (Y + SampleCount * Z);
		}
	};

	static int32 FindOrCreateVertex(
		const int32 CellX,
		const int32 CellY,
		const int32 CellZ,
		const int32 EdgeIndex,
		const FScalarFieldView& Field,
		FEdgeVertexCache& EdgeCache,
		FVoraxiaVoxelMeshData& Mesh
	)
	{
		int32& CachedVertexIndex = EdgeCache.Get(
			CellX,
			CellY,
			CellZ,
			EdgeIndex
		);

		if (CachedVertexIndex != INDEX_NONE)
		{
			return CachedVertexIndex;
		}

		const int32 FirstCornerIndex =
			EdgeCorners[EdgeIndex][0];

		const int32 SecondCornerIndex =
			EdgeCorners[EdgeIndex][1];

		const int32 FirstX =
			CellX + CornerOffsets[FirstCornerIndex][0];

		const int32 FirstY =
			CellY + CornerOffsets[FirstCornerIndex][1];

		const int32 FirstZ =
			CellZ + CornerOffsets[FirstCornerIndex][2];

		const int32 SecondX =
			CellX + CornerOffsets[SecondCornerIndex][0];

		const int32 SecondY =
			CellY + CornerOffsets[SecondCornerIndex][1];

		const int32 SecondZ =
			CellZ + CornerOffsets[SecondCornerIndex][2];

		const float FirstDensity =
			Field.Get(FirstX, FirstY, FirstZ);

		const float SecondDensity =
			Field.Get(SecondX, SecondY, SecondZ);

		const float Denominator =
			FirstDensity - SecondDensity;

		const float InterpolationAlpha =
			FMath::IsNearlyZero(Denominator)
				? 0.5f
				: FMath::Clamp(
					FirstDensity / Denominator,
					0.0f,
					1.0f
				);

		const FVector VertexPosition = FMath::Lerp(
			Field.Position(FirstX, FirstY, FirstZ),
			Field.Position(SecondX, SecondY, SecondZ),
			InterpolationAlpha
		);

		/*
		 * Positive density means solid material.
		 * The density gradient therefore points toward solid material,
		 * so its inverse is the outward normal.
		 */
		FVector VertexNormal = -FMath::Lerp(
			Field.Gradient(FirstX, FirstY, FirstZ),
			Field.Gradient(SecondX, SecondY, SecondZ),
			InterpolationAlpha
		).GetSafeNormal();

		if (VertexNormal.IsNearlyZero())
		{
			VertexNormal = VertexPosition.GetSafeNormal();
		}

		FVector TangentX = FVector::CrossProduct(
			FVector::UpVector,
			VertexNormal
		).GetSafeNormal();

		if (TangentX.IsNearlyZero())
		{
			TangentX = FVector::CrossProduct(
				FVector::RightVector,
				VertexNormal
			).GetSafeNormal();
		}

		const FVector UnitPosition =
			VertexPosition.GetSafeNormal();

		const float U =
			FMath::Atan2(UnitPosition.Y, UnitPosition.X) /
			(2.0f * PI) +
			0.5f;

		const float V =
			FMath::Asin(UnitPosition.Z) / PI +
			0.5f;

		CachedVertexIndex = Mesh.Vertices.Add(VertexPosition);

		Mesh.Normals.Add(VertexNormal);
		Mesh.UV0.Add(FVector2D(U, V));
		Mesh.VertexColors.Add(FLinearColor::White);
		Mesh.Tangents.Add(FProcMeshTangent(TangentX, false));

		return CachedVertexIndex;
	}

	static void AddTriangle(
		FVoraxiaVoxelMeshData& Mesh,
		int32 FirstVertexIndex,
		int32 SecondVertexIndex,
		int32 ThirdVertexIndex
	)
	{
		const FVector& FirstPosition =
			Mesh.Vertices[FirstVertexIndex];

		const FVector& SecondPosition =
			Mesh.Vertices[SecondVertexIndex];

		const FVector& ThirdPosition =
			Mesh.Vertices[ThirdVertexIndex];

		const FVector FaceNormal = FVector::CrossProduct(
			SecondPosition - FirstPosition,
			ThirdPosition - FirstPosition
		);

		if (FaceNormal.SizeSquared() <= SMALL_NUMBER)
		{
			return;
		}

		const FVector ExpectedOutwardNormal = (
			Mesh.Normals[FirstVertexIndex] +
			Mesh.Normals[SecondVertexIndex] +
			Mesh.Normals[ThirdVertexIndex]
		).GetSafeNormal();

		/*
		 * The classic lookup table assumes one particular inside/outside
		 * convention. This makes the triangle orientation correct even though
		 * Voraxia uses positive density for solid rock.
		 */
		if (
			FVector::DotProduct(
				FaceNormal,
				ExpectedOutwardNormal
			) > 0.0f
		)
		{
			Swap(SecondVertexIndex, ThirdVertexIndex);
		}

		Mesh.Triangles.Add(FirstVertexIndex);
		Mesh.Triangles.Add(SecondVertexIndex);
		Mesh.Triangles.Add(ThirdVertexIndex);
	}
}

void FVoraxiaVoxelMeshData::Reset()
{
	Vertices.Reset();
	Triangles.Reset();
	Normals.Reset();
	UV0.Reset();
	VertexColors.Reset();
	Tangents.Reset();
}

bool FVoraxiaMarchingCubes::BuildIsoSurface(
	const TArray<float>& DensitySamples,
	const int32 CellsPerAxis,
	const float VoxelSize,
	FVoraxiaVoxelMeshData& OutMesh
)
{
	OutMesh.Reset();

	if (CellsPerAxis < 1 || VoxelSize <= 0.0f)
	{
		return false;
	}

	const int32 SampleCount = CellsPerAxis + 1;

	const int32 RequiredDensityCount =
		SampleCount * SampleCount * SampleCount;

	if (DensitySamples.Num() != RequiredDensityCount)
	{
		return false;
	}

	const VoraxiaVoxel::MarchingCubes::FScalarFieldView Field(
		DensitySamples,
		CellsPerAxis,
		SampleCount,
		VoxelSize,
		static_cast<float>(CellsPerAxis) * VoxelSize * 0.5f
	);

	VoraxiaVoxel::MarchingCubes::FEdgeVertexCache EdgeCache(
		CellsPerAxis
	);

	const VoraxiaVoxel::MarchingCubes::FTriangleTable& TriangleTable =
		VoraxiaVoxel::MarchingCubes::GetTriangleTable();

	for (int32 CellZ = 0; CellZ < CellsPerAxis; ++CellZ)
	{
		for (int32 CellY = 0; CellY < CellsPerAxis; ++CellY)
		{
			for (int32 CellX = 0; CellX < CellsPerAxis; ++CellX)
			{
				uint8 CaseIndex = 0;

				for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
				{
					const int32 SampleX =
						CellX + VoraxiaVoxel::MarchingCubes::CornerOffsets[
							CornerIndex
						][0];

					const int32 SampleY =
						CellY + VoraxiaVoxel::MarchingCubes::CornerOffsets[
							CornerIndex
						][1];

					const int32 SampleZ =
						CellZ + VoraxiaVoxel::MarchingCubes::CornerOffsets[
							CornerIndex
						][2];

					if (Field.Get(SampleX, SampleY, SampleZ) > 0.0f)
					{
						CaseIndex |= static_cast<uint8>(
							1 << CornerIndex
						);
					}
				}

				for (
					int32 TriangleEntry = 0;
					TriangleEntry < 16;
					TriangleEntry += 3
				)
				{
					const int8 FirstEdge = TriangleTable.Get(
						CaseIndex,
						TriangleEntry
					);

					if (FirstEdge < 0)
					{
						break;
					}

					const int8 SecondEdge = TriangleTable.Get(
						CaseIndex,
						TriangleEntry + 1
					);

					const int8 ThirdEdge = TriangleTable.Get(
						CaseIndex,
						TriangleEntry + 2
					);

					check(SecondEdge >= 0 && ThirdEdge >= 0);

					const int32 FirstVertexIndex =
						VoraxiaVoxel::MarchingCubes::FindOrCreateVertex(
							CellX,
							CellY,
							CellZ,
							FirstEdge,
							Field,
							EdgeCache,
							OutMesh
						);

					const int32 SecondVertexIndex =
						VoraxiaVoxel::MarchingCubes::FindOrCreateVertex(
							CellX,
							CellY,
							CellZ,
							SecondEdge,
							Field,
							EdgeCache,
							OutMesh
						);

					const int32 ThirdVertexIndex =
						VoraxiaVoxel::MarchingCubes::FindOrCreateVertex(
							CellX,
							CellY,
							CellZ,
							ThirdEdge,
							Field,
							EdgeCache,
							OutMesh
						);

					VoraxiaVoxel::MarchingCubes::AddTriangle(
						OutMesh,
						FirstVertexIndex,
						SecondVertexIndex,
						ThirdVertexIndex
					);
				}
			}
		}
	}

	return OutMesh.Vertices.Num() > 0 &&
		OutMesh.Triangles.Num() > 0;
}


namespace VoraxiaVoxel::MarchingCubes
{
	struct FMaterialSectionBuildState final
	{
		FVoraxiaVoxelMeshData Mesh;
		FEdgeVertexCache EdgeCache;

		explicit FMaterialSectionBuildState(const int32 InCellsPerAxis)
			: EdgeCache(InCellsPerAxis)
		{
		}
	};

	static FMaterialSectionBuildState& FindOrAddMaterialSection(
		TMap<FGameplayTag, TUniquePtr<FMaterialSectionBuildState>>& InOutSections,
		const FGameplayTag MaterialTag,
		const int32 CellsPerAxis
	)
	{
		TUniquePtr<FMaterialSectionBuildState>& Section =
			InOutSections.FindOrAdd(MaterialTag);

		if (!Section)
		{
			Section = MakeUnique<FMaterialSectionBuildState>(
				CellsPerAxis
			);
		}

		return *Section;
	}
}

bool FVoraxiaMarchingCubes::BuildIsoSurfaceByMaterial(
	const TArray<float>& DensitySamples,
	const int32 CellsPerAxis,
	const float VoxelSize,
	const TArray<FGameplayTag>& CellMaterialTags,
	const FGameplayTag FallbackMaterialTag,
	TMap<FGameplayTag, FVoraxiaVoxelMeshData>& OutMeshes
)
{
	OutMeshes.Reset();

	if (
		CellsPerAxis < 1 ||
		VoxelSize <= 0.0f ||
		!FallbackMaterialTag.IsValid()
	)
	{
		return false;
	}

	const int32 SampleCount = CellsPerAxis + 1;

	const int32 RequiredDensityCount =
		SampleCount * SampleCount * SampleCount;

	const int32 RequiredCellMaterialCount =
		CellsPerAxis * CellsPerAxis * CellsPerAxis;

	if (
		DensitySamples.Num() != RequiredDensityCount ||
		CellMaterialTags.Num() != RequiredCellMaterialCount
	)
	{
		return false;
	}

	const VoraxiaVoxel::MarchingCubes::FScalarFieldView Field(
		DensitySamples,
		CellsPerAxis,
		SampleCount,
		VoxelSize,
		static_cast<float>(CellsPerAxis) * VoxelSize * 0.5f
	);

	const VoraxiaVoxel::MarchingCubes::FTriangleTable& TriangleTable =
		VoraxiaVoxel::MarchingCubes::GetTriangleTable();

	TMap<
		FGameplayTag,
		TUniquePtr<
			VoraxiaVoxel::MarchingCubes::FMaterialSectionBuildState
		>
	> MaterialSections;

	for (int32 CellZ = 0; CellZ < CellsPerAxis; ++CellZ)
	{
		for (int32 CellY = 0; CellY < CellsPerAxis; ++CellY)
		{
			for (int32 CellX = 0; CellX < CellsPerAxis; ++CellX)
			{
				uint8 CaseIndex = 0;

				for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
				{
					const int32 SampleX =
						CellX + VoraxiaVoxel::MarchingCubes::CornerOffsets[
							CornerIndex
						][0];

					const int32 SampleY =
						CellY + VoraxiaVoxel::MarchingCubes::CornerOffsets[
							CornerIndex
						][1];

					const int32 SampleZ =
						CellZ + VoraxiaVoxel::MarchingCubes::CornerOffsets[
							CornerIndex
						][2];

					if (Field.Get(SampleX, SampleY, SampleZ) > 0.0f)
					{
						CaseIndex |= static_cast<uint8>(
							1 << CornerIndex
						);
					}
				}

				if (CaseIndex == 0 || CaseIndex == 255)
				{
					continue;
				}

				const int32 CellIndex =
					CellX + CellsPerAxis * (
						CellY + CellsPerAxis * CellZ
					);

				FGameplayTag MaterialTag =
					CellMaterialTags[CellIndex];

				if (!MaterialTag.IsValid())
				{
					MaterialTag = FallbackMaterialTag;
				}

				VoraxiaVoxel::MarchingCubes::FMaterialSectionBuildState&
					Section =
						VoraxiaVoxel::MarchingCubes::FindOrAddMaterialSection(
							MaterialSections,
							MaterialTag,
							CellsPerAxis
						);

				for (
					int32 TriangleEntry = 0;
					TriangleEntry < 16;
					TriangleEntry += 3
				)
				{
					const int8 FirstEdge = TriangleTable.Get(
						CaseIndex,
						TriangleEntry
					);

					if (FirstEdge < 0)
					{
						break;
					}

					const int8 SecondEdge = TriangleTable.Get(
						CaseIndex,
						TriangleEntry + 1
					);

					const int8 ThirdEdge = TriangleTable.Get(
						CaseIndex,
						TriangleEntry + 2
					);

					check(SecondEdge >= 0 && ThirdEdge >= 0);

					const int32 FirstVertexIndex =
						VoraxiaVoxel::MarchingCubes::FindOrCreateVertex(
							CellX,
							CellY,
							CellZ,
							FirstEdge,
							Field,
							Section.EdgeCache,
							Section.Mesh
						);

					const int32 SecondVertexIndex =
						VoraxiaVoxel::MarchingCubes::FindOrCreateVertex(
							CellX,
							CellY,
							CellZ,
							SecondEdge,
							Field,
							Section.EdgeCache,
							Section.Mesh
						);

					const int32 ThirdVertexIndex =
						VoraxiaVoxel::MarchingCubes::FindOrCreateVertex(
							CellX,
							CellY,
							CellZ,
							ThirdEdge,
							Field,
							Section.EdgeCache,
							Section.Mesh
						);

					VoraxiaVoxel::MarchingCubes::AddTriangle(
						Section.Mesh,
						FirstVertexIndex,
						SecondVertexIndex,
						ThirdVertexIndex
					);
				}
			}
		}
	}

	for (
		TPair<
			FGameplayTag,
			TUniquePtr<
				VoraxiaVoxel::MarchingCubes::FMaterialSectionBuildState
			>
		>& Pair : MaterialSections
	)
	{
		if (
			!Pair.Value ||
			Pair.Value->Mesh.Vertices.IsEmpty() ||
			Pair.Value->Mesh.Triangles.IsEmpty()
		)
		{
			continue;
		}

		OutMeshes.Add(
			Pair.Key,
			MoveTemp(Pair.Value->Mesh)
		);
	}

	return !OutMeshes.IsEmpty();
}
