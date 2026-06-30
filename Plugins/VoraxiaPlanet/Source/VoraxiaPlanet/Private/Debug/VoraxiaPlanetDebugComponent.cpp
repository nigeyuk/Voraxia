// Copyright Coding Custard Studios

/**
 * @file VoraxiaPlanetDebugComponent.cpp
 * @brief Implementation of the Voraxia cube-sphere debug visualisation component.
 */

#include "Debug/VoraxiaPlanetDebugComponent.h"

#include "Core/VoraxiaPlanetDeveloperSettings.h"
#include "Planet/VoraxiaPlanetActor.h"
#include "Planet/VoraxiaPlanetDefinition.h"
#include "Planet/VoraxiaPlanetMath.h"
#include "Terrain/VoraxiaPlanetTerrainGenerator.h"
#include "Terrain/VoraxiaPlanetBaseTerrain.h"
#include "Terrain/Features/VoraxiaPlanetTectonics.h"
#include "Debug/SVoraxiaTerrainProbeWidget.h"

#include "LevelEditorViewport.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

namespace
{
	/**
	 * @brief Returns a stable short label for a cube face.
	 *
	 * @param Face Cube face to describe.
	 *
	 * @return Human-readable directional label.
	 */
	const TCHAR* GetCubeFaceLabel(const EVoraxiaCubeFace Face)
	{
		switch (Face)
		{
		case EVoraxiaCubeFace::PositiveX:
			return TEXT("+X");

		case EVoraxiaCubeFace::NegativeX:
			return TEXT("-X");

		case EVoraxiaCubeFace::PositiveY:
			return TEXT("+Y");

		case EVoraxiaCubeFace::NegativeY:
			return TEXT("-Y");

		case EVoraxiaCubeFace::PositiveZ:
			return TEXT("+Z");

		case EVoraxiaCubeFace::NegativeZ:
			return TEXT("-Z");

		default:
			return TEXT("Invalid");
		}
	}

	/**
	 * @brief Returns a distinct diagnostic colour for each cube face.
	 *
	 * @param Face Cube face to colour.
	 *
	 * @return Stable display colour for the supplied face.
	 */
	FColor GetCubeFaceColour(const EVoraxiaCubeFace Face)
	{
		switch (Face)
		{
		case EVoraxiaCubeFace::PositiveX:
			return FColor::Red;

		case EVoraxiaCubeFace::NegativeX:
			return FColor(128, 0, 0);

		case EVoraxiaCubeFace::PositiveY:
			return FColor::Green;

		case EVoraxiaCubeFace::NegativeY:
			return FColor(0, 96, 0);

		case EVoraxiaCubeFace::PositiveZ:
			return FColor::Blue;

		case EVoraxiaCubeFace::NegativeZ:
			return FColor(0, 0, 128);

		default:
			return FColor::White;
		}
	}

	/**
	 * @brief Converts a double-precision direction and scalar into an Unreal vector.
	 *
	 * @param Direction Unit or non-unit direction expressed as FVector3d.
	 * @param Scale Scalar to apply before conversion.
	 *
	 * @return Unreal-space vector suitable for debug drawing.
	 */
	FVector ToDebugVector(const FVector3d& Direction, const double Scale)
	{
		return FVector(
			Direction.X * Scale,
			Direction.Y * Scale,
			Direction.Z * Scale);
	}

	/**
	 * @brief Returns whether a double is safe for debug coordinate calculations.
	 *
	 * @param Value Value to validate.
	 *
	 * @return True when the value is finite.
	 */
	bool IsFiniteCoordinate(const double Value)
	{
		return FMath::IsFinite(Value);
	}

	/**
	 * @brief Converts a normalised direction into stable cube-face coordinates.
	 *
	 * @param UnitDirection Normalised direction from the planet centre.
	 * @param PlanetRadiusMetres Reference planet radius in metres.
	 * @param OutFace Receives the resolved cube face.
	 * @param OutUMetres Receives face-local U coordinate in metres.
	 * @param OutVMetres Receives face-local V coordinate in metres.
	 *
	 * @return True when a valid face coordinate was produced.
	 */
	bool TryConvertUnitDirectionToFaceCoordinates(
		const FVector3d& UnitDirection,
		const double PlanetRadiusMetres,
		EVoraxiaCubeFace& OutFace,
		double& OutUMetres,
		double& OutVMetres)
	{
		OutFace = EVoraxiaCubeFace::Invalid;
		OutUMetres = 0.0;
		OutVMetres = 0.0;

		if (UnitDirection.SizeSquared() <= 0.000000000001
			|| !FMath::IsFinite(PlanetRadiusMetres)
			|| PlanetRadiusMetres <= 0.0)
		{
			return false;
		}

		const FVector3d SafeUnitDirection =
			UnitDirection.GetSafeNormal();

		const double AbsoluteX = FMath::Abs(SafeUnitDirection.X);
		const double AbsoluteY = FMath::Abs(SafeUnitDirection.Y);
		const double AbsoluteZ = FMath::Abs(SafeUnitDirection.Z);

		if (AbsoluteX >= AbsoluteY && AbsoluteX >= AbsoluteZ)
		{
			OutFace = SafeUnitDirection.X >= 0.0
				? EVoraxiaCubeFace::PositiveX
				: EVoraxiaCubeFace::NegativeX;
		}
		else if (AbsoluteY >= AbsoluteZ)
		{
			OutFace = SafeUnitDirection.Y >= 0.0
				? EVoraxiaCubeFace::PositiveY
				: EVoraxiaCubeFace::NegativeY;
		}
		else
		{
			OutFace = SafeUnitDirection.Z >= 0.0
				? EVoraxiaCubeFace::PositiveZ
				: EVoraxiaCubeFace::NegativeZ;
		}

		FVector3d FaceNormal;
		FVector3d FaceUAxis;
		FVector3d FaceVAxis;

		if (!VoraxiaPlanetMath::GetCubeFaceBasis(
			OutFace,
			FaceNormal,
			FaceUAxis,
			FaceVAxis))
		{
			return false;
		}

		const double FaceNormalComponent =
			FMath::Abs(FVector3d::DotProduct(
				SafeUnitDirection,
				FaceNormal));

		if (FaceNormalComponent <= 0.000000000001)
		{
			return false;
		}

		const FVector3d CubeFacePoint =
			SafeUnitDirection / FaceNormalComponent;

		OutUMetres =
			FVector3d::DotProduct(
				CubeFacePoint,
				FaceUAxis)
			* PlanetRadiusMetres;

		OutVMetres =
			FVector3d::DotProduct(
				CubeFacePoint,
				FaceVAxis)
			* PlanetRadiusMetres;

		return VoraxiaPlanetMath::IsFaceCoordinateWithinBounds(
			OutUMetres,
			OutVMetres,
			PlanetRadiusMetres);
	}
}

UVoraxiaPlanetDebugComponent::UVoraxiaPlanetDebugComponent()
{
	/**
	 * Debug draw helpers are frame-based when lifetime is zero, so the component
	 * must tick while the debug feature is active.
	 */
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UVoraxiaPlanetDebugComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureTerrainProbeOverlay();
}

void UVoraxiaPlanetDebugComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	RemoveTerrainProbeOverlay();

	Super::EndPlay(EndPlayReason);
}

void UVoraxiaPlanetDebugComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ShouldDrawDebugVisualisation())
	{
		EnsureTerrainProbeOverlay();

		if (bEnableFreeCameraTerrainProbe
			&& bContinuouslySampleTerrainProbe)
		{
			SampleTerrainFromCurrentView();
		}
		else
		{
			TryUpdateTerrainProbeFromCameraInput();
		}

		DrawPlanetReferenceFrame();
	}
	else
	{
		bWasTerrainProbeInputDown = false;
	}
}

void UVoraxiaPlanetDebugComponent::EnsureTerrainProbeOverlay()
{
	if (!bShowTerrainProbeOverlay
		|| TerrainProbeOverlayWidget.IsValid()
		|| GEngine == nullptr
		|| GEngine->GameViewport == nullptr)
	{
		return;
	}

	TerrainProbeOverlayWidget =
		SNew(SVoraxiaTerrainProbeWidget)
		.DebugComponent(this);

	GEngine->GameViewport->AddViewportWidgetContent(
		TerrainProbeOverlayWidget.ToSharedRef(),
		100);
}

void UVoraxiaPlanetDebugComponent::RemoveTerrainProbeOverlay()
{
	if (!TerrainProbeOverlayWidget.IsValid())
	{
		return;
	}

	if (GEngine != nullptr && GEngine->GameViewport != nullptr)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(
			TerrainProbeOverlayWidget.ToSharedRef());
	}

	TerrainProbeOverlayWidget.Reset();
}

bool UVoraxiaPlanetDebugComponent::TryGetEditorFreeCameraView(
	FVector& OutCameraLocation,
	FRotator& OutCameraRotation) const
{
#if WITH_EDITOR
	/**
	 * F8 eject runs through Unreal's editor viewport client, not through the
	 * possessed player pawn. Query it first so the terrain inspector follows
	 * the active free camera. Normal player camera sampling remains a fallback.
	 */
	if (GCurrentLevelEditingViewportClient != nullptr)
	{
		OutCameraLocation =
			GCurrentLevelEditingViewportClient->GetViewLocation();

		OutCameraRotation =
			GCurrentLevelEditingViewportClient->GetViewRotation();

		return true;
	}
#endif

	return false;
}

FText UVoraxiaPlanetDebugComponent::GetTerrainProbeDisplayText() const
{
	const AVoraxiaPlanetActor* PlanetActor =
		Cast<AVoraxiaPlanetActor>(GetOwner());

	if (!IsValid(PlanetActor))
	{
		return FText::FromString(
			TEXT("VORAXIA TERRAIN PROBE\n\nPlanet actor unavailable."));
	}

	const FVoraxiaPlanetRuntimeState RuntimeState =
		PlanetActor->GetPlanetRuntimeState();

	if (!RuntimeState.IsValid())
	{
		return FText::FromString(
			TEXT("VORAXIA TERRAIN PROBE\n\nWaiting for valid runtime state."));
	}

	FVector3d UnitDirection;
	double SampleRadialDistanceMetres = 0.0;

	if (!TryGetSelectedSurfaceSample(
		RuntimeState,
		UnitDirection,
		SampleRadialDistanceMetres))
	{
		return FText::FromString(
			TEXT("VORAXIA TERRAIN PROBE\n\nSelected sample is invalid."));
	}

	FVoraxiaPlanetTerrainSample TerrainSample;

	if (!VoraxiaPlanetTerrain::SampleMacroTerrain(
		RuntimeState,
		UnitDirection,
		TerrainSample))
	{
		return FText::FromString(
			TEXT("VORAXIA TERRAIN PROBE\n\nTerrain sample failed."));
	}

	const double SurfaceRadiusMetres =
		RuntimeState.RadiusMetres
		+ TerrainSample.HeightMetres;

	const double OffsetFromTerrainMetres =
		DebugSurfaceSampleAltitudeMetres
		- TerrainSample.HeightMetres;

	FString Readout = FString::Printf(
		TEXT("VORAXIA TERRAIN PROBE\n")
		TEXT("Live centre-reticle sample\n\n")
		TEXT("Face                 %s\n")
		TEXT("U / V                %+.1f / %+.1f m\n")
		TEXT("Terrain Height       %+.1f m\n")
		TEXT("Surface Radius       %.1f m\n")
		TEXT("Offset From Terrain  %+.1f m\n\n")
		TEXT("Continentalness      %.3f\n")
		TEXT("Mountainness         %.3f"),
		GetCubeFaceLabel(DebugSurfaceSampleFace),
		DebugSurfaceSampleUMetres,
		DebugSurfaceSampleVMetres,
		TerrainSample.HeightMetres,
		SurfaceRadiusMetres,
		OffsetFromTerrainMetres,
		TerrainSample.Continentalness,
		TerrainSample.Mountainness);

	if (RuntimeState.GeneratorVersion == 2)
	{
		FVoraxiaPlanetBaseTerrainSample BaseTerrain;
		FVoraxiaPlanetTectonicsSample Tectonics;

		if (VoraxiaPlanetBaseTerrain::SampleBaseTerrain(
			RuntimeState,
			UnitDirection,
			BaseTerrain)
			&& VoraxiaPlanetTectonics::SampleTectonics(
				RuntimeState,
				UnitDirection,
				BaseTerrain.Continentalness,
				Tectonics))
		{
			Readout += FString::Printf(
				TEXT("\n\nTECTONICS\n")
				TEXT("Uplift               %+.1f m\n")
				TEXT("Rift Depth           %+.1f m\n")
				TEXT("Activity             %.3f"),
				Tectonics.UpliftMetres,
				Tectonics.RiftDepthMetres,
				Tectonics.Activity);
		}
	}

	return FText::FromString(Readout);
}

bool UVoraxiaPlanetDebugComponent::ShouldShowTerrainProbeOverlay() const
{
	return bShowTerrainProbeOverlay
		&& ShouldDrawDebugVisualisation();
}

bool UVoraxiaPlanetDebugComponent::SampleTerrainFromCurrentView()
{
	AVoraxiaPlanetActor* PlanetActor =
		Cast<AVoraxiaPlanetActor>(GetOwner());

	if (!IsValid(PlanetActor))
	{
		return false;
	}

	const FVoraxiaPlanetRuntimeState RuntimeState =
		PlanetActor->GetPlanetRuntimeState();

	if (!RuntimeState.IsValid())
	{
		return false;
	}

	FVector3d UnitDirection;

	if (!TryGetPreviewSphereDirectionFromCurrentView(UnitDirection))
	{
		return false;
	}

	EVoraxiaCubeFace ResolvedFace = EVoraxiaCubeFace::Invalid;
	double ResolvedUMetres = 0.0;
	double ResolvedVMetres = 0.0;

	if (!TryConvertUnitDirectionToFaceCoordinates(
		UnitDirection,
		RuntimeState.RadiusMetres,
		ResolvedFace,
		ResolvedUMetres,
		ResolvedVMetres))
	{
		return false;
	}

	FVoraxiaPlanetTerrainSample TerrainSample;

	if (!VoraxiaPlanetTerrain::SampleMacroTerrain(
		RuntimeState,
		UnitDirection,
		TerrainSample))
	{
		return false;
	}

	/**
	 * Set selected altitude to the sampled terrain height, placing the marker
	 * on generated ground rather than on the mathematical reference sphere.
	 */
	DebugSurfaceSampleFace = ResolvedFace;
	DebugSurfaceSampleUMetres = ResolvedUMetres;
	DebugSurfaceSampleVMetres = ResolvedVMetres;
	DebugSurfaceSampleAltitudeMetres = TerrainSample.HeightMetres;

	return true;
}

bool UVoraxiaPlanetDebugComponent::TryUpdateTerrainProbeFromCameraInput()
{
	if (!bEnableFreeCameraTerrainProbe
		|| !bProbeTerrainWithMiddleMouseButton)
	{
		bWasTerrainProbeInputDown = false;
		return false;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		bWasTerrainProbeInputDown = false;
		return false;
	}

	APlayerController* PlayerController =
		UGameplayStatics::GetPlayerController(World, 0);

	if (!IsValid(PlayerController))
	{
		bWasTerrainProbeInputDown = false;
		return false;
	}

	const bool bMiddleMouseDown =
		PlayerController->IsInputKeyDown(EKeys::MiddleMouseButton);

	const bool bShouldSample =
		bMiddleMouseDown
		&& !bWasTerrainProbeInputDown;

	bWasTerrainProbeInputDown = bMiddleMouseDown;

	return bShouldSample
		? SampleTerrainFromCurrentView()
		: false;
}

bool UVoraxiaPlanetDebugComponent::
TryGetPreviewSphereDirectionFromCurrentView(
	FVector3d& OutUnitDirection) const
{
	OutUnitDirection = FVector3d::ZeroVector;

	const AVoraxiaPlanetActor* PlanetActor =
		Cast<AVoraxiaPlanetActor>(GetOwner());

	UWorld* World = GetWorld();

	if (!IsValid(PlanetActor) || !IsValid(World))
	{
		return false;
	}

	FVector CameraLocation;
	FRotator CameraRotation;

	if (!TryGetEditorFreeCameraView(
		CameraLocation,
		CameraRotation))
	{
		APlayerController* PlayerController =
			UGameplayStatics::GetPlayerController(World, 0);

		if (!IsValid(PlayerController))
		{
			return false;
		}

		PlayerController->GetPlayerViewPoint(
			CameraLocation,
			CameraRotation);
	}

	const FVector3d RayOrigin =
		FVector3d(CameraLocation)
		- FVector3d(PlanetActor->GetActorLocation());

	const FVector3d RayDirection =
		FVector3d(CameraRotation.Vector()).GetSafeNormal();

	const double PreviewRadiusCentimetres =
		FMath::Max(100.0, DebugPreviewRadiusCentimetres);

	if (RayDirection.SizeSquared() <= 0.000000000001)
	{
		return false;
	}

	const double RayOriginProjection =
		FVector3d::DotProduct(RayOrigin, RayDirection);

	const double RayOriginLengthSquared =
		RayOrigin.SizeSquared();

	const double Discriminant =
		(RayOriginProjection * RayOriginProjection)
		- (RayOriginLengthSquared
			- (PreviewRadiusCentimetres * PreviewRadiusCentimetres));

	if (Discriminant < 0.0)
	{
		return false;
	}

	const double Root = FMath::Sqrt(Discriminant);
	const double NearDistance = -RayOriginProjection - Root;
	const double FarDistance = -RayOriginProjection + Root;

	const double HitDistance =
		NearDistance > 0.0
		? NearDistance
		: FarDistance > 0.0
			? FarDistance
			: -1.0;

	if (HitDistance <= 0.0)
	{
		return false;
	}

	const FVector3d HitPoint =
		RayOrigin + (RayDirection * HitDistance);

	if (HitPoint.SizeSquared() <= 0.000000000001)
	{
		return false;
	}

	OutUnitDirection = HitPoint.GetSafeNormal();

	return true;
}

bool UVoraxiaPlanetDebugComponent::ShouldDrawDebugVisualisation() const
{
	if (!bDrawPlanetReferenceFrame)
	{
		return false;
	}

	const UVoraxiaPlanetDeveloperSettings* Settings =
		GetDefault<UVoraxiaPlanetDeveloperSettings>();

	if (!IsValid(Settings) || !Settings->bEnablePlanetDebugging)
	{
		return false;
	}

	const AVoraxiaPlanetActor* PlanetActor =
		Cast<AVoraxiaPlanetActor>(GetOwner());

	return IsValid(PlanetActor)
		&& PlanetActor->HasValidPlanetRuntimeState();
}

void UVoraxiaPlanetDebugComponent::DrawPlanetReferenceFrame()
{
	AVoraxiaPlanetActor* PlanetActor =
		Cast<AVoraxiaPlanetActor>(GetOwner());

	if (!IsValid(PlanetActor))
	{
		return;
	}

	const FVoraxiaPlanetRuntimeState RuntimeState =
		PlanetActor->GetPlanetRuntimeState();

	if (!RuntimeState.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	const FVector Origin = PlanetActor->GetActorLocation();

	const double PreviewRadiusCentimetres =
		FMath::Max(100.0, DebugPreviewRadiusCentimetres);

	const double ArrowLengthCentimetres =
		PreviewRadiusCentimetres
		* FMath::Clamp(DebugArrowLengthRatio, 0.05, 2.0);

	const float ArrowHeadSize =
		static_cast<float>(FMath::Max(20.0, ArrowLengthCentimetres * 0.15));

	if (bDrawReferenceSphere)
	{
		DrawDebugSphere(
			World,
			Origin,
			static_cast<float>(PreviewRadiusCentimetres),
			FMath::Clamp(DebugSphereSegments, 8, 64),
			FColor::Cyan,
			false,
			0.0f,
			SDPG_World,
			DebugLineThickness);
	}

	if (bDrawSourceCube)
	{
		DrawDebugBox(
			World,
			Origin,
			FVector(
				PreviewRadiusCentimetres,
				PreviewRadiusCentimetres,
				PreviewRadiusCentimetres),
			FColor(96, 96, 96),
			false,
			0.0f,
			SDPG_World,
			DebugLineThickness * 0.75f);
	}

	DrawDebugPoint(
		World,
		Origin,
		16.0f,
		FColor::White,
		false,
		0.0f,
		SDPG_World);

	/**
	 * This order matches EVoraxiaCubeFace and remains explicit for diagnostic
	 * clarity. Do not infer face order from enum arithmetic here.
	 */
	static const EVoraxiaCubeFace Faces[] =
	{
		EVoraxiaCubeFace::PositiveX,
		EVoraxiaCubeFace::NegativeX,
		EVoraxiaCubeFace::PositiveY,
		EVoraxiaCubeFace::NegativeY,
		EVoraxiaCubeFace::PositiveZ,
		EVoraxiaCubeFace::NegativeZ
	};

	for (const EVoraxiaCubeFace Face : Faces)
	{
		FVector3d UnitDirection;

		/**
		 * Sampling the centre of each face deliberately exercises the same
		 * cube-face-to-sphere conversion used by future terrain generation.
		 */
		if (!VoraxiaPlanetMath::FaceCoordinatesToUnitDirection(
			Face,
			0.0,
			0.0,
			RuntimeState.RadiusMetres,
			UnitDirection))
		{
			continue;
		}

		FVector3d FaceNormal;
		FVector3d FaceUAxis;
		FVector3d FaceVAxis;

		if (!VoraxiaPlanetMath::GetCubeFaceBasis(
			Face,
			FaceNormal,
			FaceUAxis,
			FaceVAxis))
		{
			continue;
		}

		const FColor FaceColour = GetCubeFaceColour(Face);

		const FVector FaceCentre =
			Origin + ToDebugVector(UnitDirection, PreviewRadiusCentimetres);

		const FVector NormalEnd =
			FaceCentre + ToDebugVector(UnitDirection, ArrowLengthCentimetres);

		if (bDrawFaceNormals)
		{
			DrawDebugDirectionalArrow(
				World,
				FaceCentre,
				NormalEnd,
				ArrowHeadSize,
				FaceColour,
				false,
				0.0f,
				SDPG_World,
				DebugLineThickness);
		}

		if (bDrawGravityDirections)
		{
			const FVector GravityEnd =
				FaceCentre - ToDebugVector(
					UnitDirection,
					ArrowLengthCentimetres * 0.75);

			DrawDebugDirectionalArrow(
				World,
				FaceCentre,
				GravityEnd,
				ArrowHeadSize,
				FColor::White,
				false,
				0.0f,
				SDPG_World,
				DebugLineThickness);
		}

		if (bDrawFaceTangentAxes)
		{
			const FVector UAxisEnd =
				FaceCentre + ToDebugVector(
					FaceUAxis,
					ArrowLengthCentimetres * 0.70);

			const FVector VAxisEnd =
				FaceCentre + ToDebugVector(
					FaceVAxis,
					ArrowLengthCentimetres * 0.70);

			DrawDebugDirectionalArrow(
				World,
				FaceCentre,
				UAxisEnd,
				ArrowHeadSize * 0.75f,
				FColor::Yellow,
				false,
				0.0f,
				SDPG_World,
				DebugLineThickness);

			DrawDebugDirectionalArrow(
				World,
				FaceCentre,
				VAxisEnd,
				ArrowHeadSize * 0.75f,
				FColor::Magenta,
				false,
				0.0f,
				SDPG_World,
				DebugLineThickness);
		}

		if (bDrawFaceLabels)
		{
			const FVector LabelPosition =
				FaceCentre + ToDebugVector(
					UnitDirection,
					ArrowLengthCentimetres * 1.15);

			DrawDebugString(
				World,
				LabelPosition,
				GetCubeFaceLabel(Face),
				nullptr,
				FaceColour,
				0.0f,
				false,
				1.25f);
		}
	}

	if (bDrawChunkGrid)
	{
		DrawChunkGrid(
			World,
			Origin,
			RuntimeState,
			PreviewRadiusCentimetres);
	}
	
	if (bDrawSelectedSurfaceSample)
	{
		DrawSelectedSurfaceSample(
			World,
			Origin,
			RuntimeState,
			PreviewRadiusCentimetres,
			ArrowLengthCentimetres,
			ArrowHeadSize);
	}
}

bool UVoraxiaPlanetDebugComponent::TryGetSelectedSurfaceSample(
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	FVector3d& OutUnitDirection,
	double& OutRadialDistanceMetres) const
{
	OutUnitDirection = FVector3d::ZeroVector;
	OutRadialDistanceMetres = 0.0;

	if (!RuntimeState.IsValid()
		|| !VoraxiaPlanet::IsValidCubeFace(DebugSurfaceSampleFace)
		|| !IsFiniteCoordinate(DebugSurfaceSampleUMetres)
		|| !IsFiniteCoordinate(DebugSurfaceSampleVMetres)
		|| !IsFiniteCoordinate(DebugSurfaceSampleAltitudeMetres))
	{
		return false;
	}

	if (!VoraxiaPlanetMath::IsFaceCoordinateWithinBounds(
		DebugSurfaceSampleUMetres,
		DebugSurfaceSampleVMetres,
		RuntimeState.RadiusMetres))
	{
		return false;
	}

	const double SampleRadialDistanceMetres =
		RuntimeState.RadiusMetres + DebugSurfaceSampleAltitudeMetres;

	/**
	 * A valid surface or underground sample must remain outside the mathematical
	 * planet centre. This protects us from nonsense debug input such as an
	 * altitude more negative than the full planet radius.
	 */
	if (!IsFiniteCoordinate(SampleRadialDistanceMetres)
		|| SampleRadialDistanceMetres <= 0.0)
	{
		return false;
	}

	if (!VoraxiaPlanetMath::FaceCoordinatesToUnitDirection(
		DebugSurfaceSampleFace,
		DebugSurfaceSampleUMetres,
		DebugSurfaceSampleVMetres,
		RuntimeState.RadiusMetres,
		OutUnitDirection))
	{
		return false;
	}

	OutRadialDistanceMetres = SampleRadialDistanceMetres;

	return true;
}

void UVoraxiaPlanetDebugComponent::DrawSelectedSurfaceSample(
	UWorld* World,
	const FVector& Origin,
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const double PreviewRadiusCentimetres,
	const double ArrowLengthCentimetres,
	const float ArrowHeadSize) const
{
	if (!IsValid(World))
	{
		return;
	}

	FVector3d UnitDirection;
	double SampleRadialDistanceMetres = 0.0;

	if (!TryGetSelectedSurfaceSample(
		RuntimeState,
		UnitDirection,
		SampleRadialDistanceMetres))
	{
		return;
	}

	/**
	 * The debug preview has its own small visual radius, so scale the selected
	 * radial distance proportionally. At zero altitude this lands exactly on
	 * the cyan reference sphere. Positive and negative altitude visibly move
	 * the orange marker outward or inward.
	 */
	const double PreviewRadialDistanceCentimetres =
		PreviewRadiusCentimetres
		* (SampleRadialDistanceMetres / RuntimeState.RadiusMetres);

	if (!IsFiniteCoordinate(PreviewRadialDistanceCentimetres)
		|| PreviewRadialDistanceCentimetres <= 0.0)
	{
		return;
	}

	const FVector SampleLocation =
		Origin + ToDebugVector(
			UnitDirection,
			PreviewRadialDistanceCentimetres);

	const double SampleArrowLengthCentimetres =
		ArrowLengthCentimetres * 0.90;

	const float SampleArrowHeadSize =
		ArrowHeadSize * 0.85f;

	const FColor SampleColour(255, 165, 0);

	if (bDrawSurfaceSampleRadialLine)
	{
		DrawDebugLine(
			World,
			Origin,
			SampleLocation,
			SampleColour,
			false,
			0.0f,
			SDPG_World,
			DebugLineThickness * 1.25f);
	}

	DrawDebugSphere(
		World,
		SampleLocation,
		static_cast<float>(
			FMath::Max(5.0, DebugSurfaceSampleMarkerRadiusCentimetres)),
		12,
		SampleColour,
		false,
		0.0f,
		SDPG_World,
		DebugLineThickness * 1.5f);

	DrawDebugPoint(
		World,
		SampleLocation,
		static_cast<float>(
			FMath::Max(10.0, DebugSurfaceSampleMarkerRadiusCentimetres)),
		SampleColour,
		false,
		0.0f,
		SDPG_World);

	if (bDrawSurfaceSampleUpDirection)
	{
		const FVector UpEnd =
			SampleLocation + ToDebugVector(
				UnitDirection,
				SampleArrowLengthCentimetres);

		DrawDebugDirectionalArrow(
			World,
			SampleLocation,
			UpEnd,
			SampleArrowHeadSize,
			FColor::Green,
			false,
			0.0f,
			SDPG_World,
			DebugLineThickness * 1.5f);
	}

	if (bDrawSurfaceSampleGravityDirection)
	{
		const FVector GravityEnd =
			SampleLocation - ToDebugVector(
				UnitDirection,
				SampleArrowLengthCentimetres * 0.75);

		DrawDebugDirectionalArrow(
			World,
			SampleLocation,
			GravityEnd,
			SampleArrowHeadSize,
			FColor::White,
			false,
			0.0f,
			SDPG_World,
			DebugLineThickness * 1.5f);
	}

	if (bDrawSurfaceSampleLabel)
	{
		FString SurfaceSampleLabel = FString::Printf(
			TEXT("Sample %s | U=%.1f m | V=%.1f m | Alt=%.1f m"),
			GetCubeFaceLabel(DebugSurfaceSampleFace),
			DebugSurfaceSampleUMetres,
			DebugSurfaceSampleVMetres,
			DebugSurfaceSampleAltitudeMetres);

		if (bShowTerrainProbeMetricsInLabel)
		{
			FVoraxiaPlanetTerrainSample TerrainSample;

			if (VoraxiaPlanetTerrain::SampleMacroTerrain(
				RuntimeState,
				UnitDirection,
				TerrainSample))
			{
				const double SurfaceRadiusMetres =
					RuntimeState.RadiusMetres
					+ TerrainSample.HeightMetres;

				const double OffsetFromTerrainMetres =
					DebugSurfaceSampleAltitudeMetres
					- TerrainSample.HeightMetres;

				SurfaceSampleLabel += FString::Printf(
					TEXT("\nTerrain Height: %+.1f m | Surface Radius: %.1f m")
					TEXT("\nOffset From Terrain: %+.1f m")
					TEXT("\nContinentalness: %.3f | Mountainness: %.3f"),
					TerrainSample.HeightMetres,
					SurfaceRadiusMetres,
					OffsetFromTerrainMetres,
					TerrainSample.Continentalness,
					TerrainSample.Mountainness);
			}
		}

		const FVector LabelPosition =
			SampleLocation + ToDebugVector(
				UnitDirection,
				SampleArrowLengthCentimetres * 1.25);

		DrawDebugString(
			World,
			LabelPosition,
			SurfaceSampleLabel,
			nullptr,
			SampleColour,
			0.0f,
			false,
			1.15f);
	}
}

void UVoraxiaPlanetDebugComponent::DrawChunkGrid(
	UWorld* World,
	const FVector& Origin,
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const double PreviewRadiusCentimetres) const
{
	if (!IsValid(World) || !RuntimeState.IsValid())
	{
		return;
	}

	/**
	 * Level 4 produces 16 x 16 patches per face, which is already more than
	 * enough for a useful visual check without creating a debug-line blizzard.
	 */
	const int32 GridLevel =
		FMath::Clamp(DebugChunkGridLevel, 0, 4);

	const int32 SamplesPerEdge =
		FMath::Clamp(DebugChunkGridSamplesPerEdge, 2, 32);

	/**
	 * Keep this explicit rather than relying on enum arithmetic. The order
	 * matches EVoraxiaCubeFace and makes debugging output deterministic.
	 */
	static const EVoraxiaCubeFace Faces[] =
	{
		EVoraxiaCubeFace::PositiveX,
		EVoraxiaCubeFace::NegativeX,
		EVoraxiaCubeFace::PositiveY,
		EVoraxiaCubeFace::NegativeY,
		EVoraxiaCubeFace::PositiveZ,
		EVoraxiaCubeFace::NegativeZ
	};

	for (const EVoraxiaCubeFace Face : Faces)
	{
		DrawFaceChunkGrid(
			World,
			Origin,
			RuntimeState,
			Face,
			GridLevel,
			PreviewRadiusCentimetres,
			SamplesPerEdge);
	}
}

void UVoraxiaPlanetDebugComponent::DrawFaceChunkGrid(
	UWorld* World,
	const FVector& Origin,
	const FVoraxiaPlanetRuntimeState& RuntimeState,
	const EVoraxiaCubeFace Face,
	const int32 GridLevel,
	const double PreviewRadiusCentimetres,
	const int32 SamplesPerEdge) const
{
	if (!IsValid(World)
		|| !RuntimeState.IsValid()
		|| !VoraxiaPlanet::IsValidCubeFace(Face))
	{
		return;
	}

	const int32 PatchesPerAxis =
		FVoraxiaPlanetChunkId::GetPatchesPerAxis(GridLevel);

	if (PatchesPerAxis <= 0)
	{
		return;
	}

	const double RadiusMetres = RuntimeState.RadiusMetres;
	const FColor FaceColour = GetCubeFaceColour(Face);

	const float GridLineThickness =
		FMath::Max(0.5f, DebugLineThickness * 1.15f);

	/**
	 * A level-N face contains 2^N patches per axis, therefore it has
	 * (2^N + 1) boundary lines in both U and V directions.
	 *
	 * Boundary index 0 is -RadiusMetres.
	 * Boundary index PatchesPerAxis is +RadiusMetres.
	 */
	for (int32 BoundaryIndex = 0;
		BoundaryIndex <= PatchesPerAxis;
		++BoundaryIndex)
	{
		const double BoundaryAlpha =
			static_cast<double>(BoundaryIndex)
			/ static_cast<double>(PatchesPerAxis);

		const double BoundaryCoordinateMetres =
			FMath::Lerp(
				-RadiusMetres,
				RadiusMetres,
				BoundaryAlpha);

		/**
		 * Constant V line: U travels from the left edge of the cube face
		 * to the right edge.
		 */
		DrawFaceCoordinateCurve(
			World,
			Origin,
			Face,
			-RadiusMetres,
			BoundaryCoordinateMetres,
			RadiusMetres,
			BoundaryCoordinateMetres,
			RadiusMetres,
			PreviewRadiusCentimetres,
			SamplesPerEdge,
			FaceColour,
			GridLineThickness);

		/**
		 * Constant U line: V travels from the bottom edge of the cube face
		 * to the top edge.
		 */
		DrawFaceCoordinateCurve(
			World,
			Origin,
			Face,
			BoundaryCoordinateMetres,
			-RadiusMetres,
			BoundaryCoordinateMetres,
			RadiusMetres,
			RadiusMetres,
			PreviewRadiusCentimetres,
			SamplesPerEdge,
			FaceColour,
			GridLineThickness);
	}
}

void UVoraxiaPlanetDebugComponent::DrawFaceCoordinateCurve(
	UWorld* World,
	const FVector& Origin,
	const EVoraxiaCubeFace Face,
	const double StartUMetres,
	const double StartVMetres,
	const double EndUMetres,
	const double EndVMetres,
	const double ReferenceRadiusMetres,
	const double PreviewRadiusCentimetres,
	const int32 SamplesPerEdge,
	const FColor& LineColour,
	const float LineThickness) const
{
	if (!IsValid(World)
		|| !VoraxiaPlanet::IsValidCubeFace(Face)
		|| SamplesPerEdge < 1)
	{
		return;
	}

	bool bHasPreviousPoint = false;
	FVector PreviousPoint = FVector::ZeroVector;

	for (int32 SegmentIndex = 0;
		SegmentIndex <= SamplesPerEdge;
		++SegmentIndex)
	{
		const double SegmentAlpha =
			static_cast<double>(SegmentIndex)
			/ static_cast<double>(SamplesPerEdge);

		const double FaceUMetres =
			FMath::Lerp(
				StartUMetres,
				EndUMetres,
				SegmentAlpha);

		const double FaceVMetres =
			FMath::Lerp(
				StartVMetres,
				EndVMetres,
				SegmentAlpha);

		FVector3d UnitDirection;

		/**
		 * This is the important part: every boundary sample is transformed by
		 * the same cube-face-to-sphere conversion the terrain system will use.
		 *
		 * We are not drawing a flat grid onto a sphere-shaped prop.
		 */
		if (!VoraxiaPlanetMath::FaceCoordinatesToUnitDirection(
			Face,
			FaceUMetres,
			FaceVMetres,
			ReferenceRadiusMetres,
			UnitDirection))
		{
			bHasPreviousPoint = false;
			continue;
		}

		const FVector CurrentPoint =
			Origin + ToDebugVector(
				UnitDirection,
				PreviewRadiusCentimetres);

		if (bHasPreviousPoint)
		{
			DrawDebugLine(
				World,
				PreviousPoint,
				CurrentPoint,
				LineColour,
				false,
				0.0f,
				SDPG_World,
				LineThickness);
		}

		PreviousPoint = CurrentPoint;
		bHasPreviousPoint = true;
	}
}