// Copyright 2026 Coding Custard Studios.

#include "Widgets/SVoraxiaCameraDebugPanel.h"

#include "Styling/CoreStyle.h"
#include "VoraxiaCameraComponent.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SVoraxiaCameraDebugPanel::Construct(const FArguments& InArgs)
{
	CameraComponent = InArgs._CameraComponent;
	PanelWidth = InArgs._PanelWidth;

	const FLinearColor BackgroundColor(0.035f, 0.035f, 0.035f, 0.72f);

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(PanelWidth)
		.Padding(FMargin(12.0f, 12.0f, 0.0f, 12.0f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(BackgroundColor)
			.Padding(12.0f)
			[
				SNew(SScrollBox)

				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						MakeHeading(FText::FromString(TEXT("VORAXIA CAMERA")))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeDivider()
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 4.0f)
					[
						MakeHeading(FText::FromString(TEXT("State")))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Mode")),
							TAttribute<FText>::CreateLambda([this]() { return GetModeText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Collision")),
							TAttribute<FText>::CreateLambda([this]() { return GetCollisionText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 4.0f)
					[
						MakeHeading(FText::FromString(TEXT("Focus")))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Active")),
							TAttribute<FText>::CreateLambda([this]() { return GetFocusActiveText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Has Target")),
							TAttribute<FText>::CreateLambda([this]() { return GetFocusHasTargetText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Alpha")),
							TAttribute<FText>::CreateLambda([this]() { return GetFocusAlphaText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Target")),
							TAttribute<FText>::CreateLambda([this]() { return GetFocusTargetText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Location")),
							TAttribute<FText>::CreateLambda([this]() { return GetFocusLocationText(); })
						)
					]


					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 4.0f)
					[
						MakeHeading(FText::FromString(TEXT("Distance")))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Desired")),
							TAttribute<FText>::CreateLambda([this]() { return GetDesiredDistanceText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Effective")),
							TAttribute<FText>::CreateLambda([this]() { return GetEffectiveDistanceText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Collision Dist")),
							TAttribute<FText>::CreateLambda([this]() { return GetCollisionDistanceText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 4.0f)
					[
						MakeHeading(FText::FromString(TEXT("Rotation / FOV")))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Desired Rot")),
							TAttribute<FText>::CreateLambda([this]() { return GetDesiredRotationText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Smoothed Rot")),
							TAttribute<FText>::CreateLambda([this]() { return GetSmoothedRotationText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("FOV")),
							TAttribute<FText>::CreateLambda([this]() { return GetFOVText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 4.0f)
					[
						MakeHeading(FText::FromString(TEXT("Locations")))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Pivot")),
							TAttribute<FText>::CreateLambda([this]() { return GetPivotLocationText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Desired Cam")),
							TAttribute<FText>::CreateLambda([this]() { return GetDesiredCameraLocationText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeMetric(
							FText::FromString(TEXT("Final Cam")),
							TAttribute<FText>::CreateLambda([this]() { return GetFinalCameraLocationText(); })
						)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 4.0f)
					[
						MakeHeading(FText::FromString(TEXT("Summary")))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return GetSummaryText(); })
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
						.AutoWrapText(true)
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SVoraxiaCameraDebugPanel::MakeHeading(const FText& Text) const
{
	return SNew(STextBlock)
		.Text(Text)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		.ColorAndOpacity(FSlateColor(FLinearColor::White));
}

TSharedRef<SWidget> SVoraxiaCameraDebugPanel::MakeMetric(
	const FText& Label,
	TAttribute<FText> ValueText
) const
{
	return SNew(SVerticalBox)

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(0.0f, 2.0f)
	[
		SNew(STextBlock)
		.Text(Label)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		.ColorAndOpacity(FSlateColor(FLinearColor::White))
	]

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(8.0f, 0.0f, 0.0f, 3.0f)
	[
		SNew(STextBlock)
		.Text(ValueText)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
		.ColorAndOpacity(FSlateColor(FLinearColor::White))
	];
}

TSharedRef<SWidget> SVoraxiaCameraDebugPanel::MakeDivider() const
{
	return SNew(SSeparator)
		.Thickness(1.0f);
}

FText SVoraxiaCameraDebugPanel::GetModeText() const
{
	return FText::FromString(TEXT("Gameplay"));
}

FText SVoraxiaCameraDebugPanel::GetCollisionText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	if (!Component)
	{
		return FText::FromString(TEXT("No Component"));
	}

	return Component->IsCameraCollisionBlocked()
		? FText::FromString(TEXT("Blocked"))
		: FText::FromString(TEXT("Clear"));
}

FText SVoraxiaCameraDebugPanel::GetFocusActiveText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	if (!Component)
	{
		return FText::FromString(TEXT("No Component"));
	}

	return Component->IsFocusActive()
		? FText::FromString(TEXT("Yes"))
		: FText::FromString(TEXT("No"));
}

FText SVoraxiaCameraDebugPanel::GetFocusHasTargetText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	if (!Component)
	{
		return FText::FromString(TEXT("No Component"));
	}

	return Component->HasFocusTarget()
		? FText::FromString(TEXT("Yes"))
		: FText::FromString(TEXT("No"));
}

FText SVoraxiaCameraDebugPanel::GetFocusAlphaText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(FString::Printf(TEXT("%.2f"), Component->GetCurrentFocusAlpha()))
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetFocusTargetText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(Component->GetCurrentFocusTargetName())
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetFocusLocationText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	if (!Component || !Component->IsFocusActive())
	{
		return FText::FromString(TEXT("-"));
	}

	return FText::FromString(Component->GetCurrentFocusLocation().ToCompactString());
}


FText SVoraxiaCameraDebugPanel::GetDesiredDistanceText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(FString::Printf(TEXT("%.1f"), Component->GetDesiredDistanceFromPivot()))
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetEffectiveDistanceText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(FString::Printf(TEXT("%.1f"), Component->GetEffectiveDistanceFromPivot()))
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetCollisionDistanceText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(FString::Printf(TEXT("%.1f"), Component->GetCurrentCollisionDistanceFromPivot()))
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetFOVText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(FString::Printf(TEXT("%.1f"), Component->GetCurrentFOV()))
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetDesiredRotationText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	if (!Component)
	{
		return FText::FromString(TEXT("-"));
	}

	const FRotator Rotation = Component->GetDesiredCameraRotation();

	return FText::FromString(
		FString::Printf(TEXT("P %.1f | Y %.1f | R %.1f"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll)
	);
}

FText SVoraxiaCameraDebugPanel::GetSmoothedRotationText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	if (!Component)
	{
		return FText::FromString(TEXT("-"));
	}

	const FRotator Rotation = Component->GetSmoothedCameraRotation();

	return FText::FromString(
		FString::Printf(TEXT("P %.1f | Y %.1f | R %.1f"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll)
	);
}

FText SVoraxiaCameraDebugPanel::GetPivotLocationText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(Component->GetLastPivotLocation().ToCompactString())
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetDesiredCameraLocationText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(Component->GetLastDesiredCameraLocation().ToCompactString())
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetFinalCameraLocationText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(Component->GetLastFinalCameraLocation().ToCompactString())
		: FText::FromString(TEXT("-"));
}

FText SVoraxiaCameraDebugPanel::GetSummaryText() const
{
	const UVoraxiaCameraComponent* Component = CameraComponent.Get();

	return Component
		? FText::FromString(Component->GetCameraDebugSummary())
		: FText::FromString(TEXT("No component"));
}
