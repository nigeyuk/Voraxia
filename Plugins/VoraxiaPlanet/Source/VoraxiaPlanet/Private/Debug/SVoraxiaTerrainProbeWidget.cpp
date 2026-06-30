// Copyright Coding Custard Studios

/**
 * @file SVoraxiaTerrainProbeWidget.cpp
 * @brief Implementation of the local Voraxia terrain-probe Slate overlay.
 */

#include "Debug/SVoraxiaTerrainProbeWidget.h"

#include "Debug/VoraxiaPlanetDebugComponent.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "LevelEditorViewport.h"
#include "Widgets/Text/STextBlock.h"

void SVoraxiaTerrainProbeWidget::Construct(
	const FArguments& InArgs)
{
	DebugComponent = InArgs._DebugComponent;

	ChildSlot
	[
		SNew(SBox)
		.Padding(FMargin(20.0f, 20.0f, 0.0f, 0.0f))
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SNew(SBorder)
			.Padding(FMargin(12.0f))
			.BorderBackgroundColor(
				FLinearColor(
					0.03f,
					0.03f,
					0.03f,
					0.82f))
			[
				SNew(STextBlock)
				.Text(this, &SVoraxiaTerrainProbeWidget::GetProbeText)
				.ColorAndOpacity(
					FSlateColor(
						FLinearColor(
							0.95f,
							0.95f,
							0.95f,
							1.0f)))
				.AutoWrapText(false)
			]
		]
	];
}

FText SVoraxiaTerrainProbeWidget::GetProbeText() const
{
	const UVoraxiaPlanetDebugComponent* Component =
		DebugComponent.Get();

	return IsValid(Component)
		? Component->GetTerrainProbeDisplayText()
		: FText::FromString(
			TEXT("VORAXIA TERRAIN PROBE\n\nDebug component unavailable."));
}

EVisibility SVoraxiaTerrainProbeWidget::GetProbeVisibility() const
{
	const UVoraxiaPlanetDebugComponent* Component =
		DebugComponent.Get();

	return IsValid(Component)
		&& Component->ShouldShowTerrainProbeOverlay()
		? EVisibility::Visible
		: EVisibility::Collapsed;
}
