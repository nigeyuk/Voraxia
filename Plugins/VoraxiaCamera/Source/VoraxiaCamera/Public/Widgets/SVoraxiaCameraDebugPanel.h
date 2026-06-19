// Copyright 2026 Coding Custard Studios.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UVoraxiaCameraComponent;

class SVoraxiaCameraDebugPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVoraxiaCameraDebugPanel)
		: _PanelWidth(420.0f)
	{
	}

	SLATE_ARGUMENT(TWeakObjectPtr<UVoraxiaCameraComponent>, CameraComponent)
	SLATE_ARGUMENT(float, PanelWidth)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TWeakObjectPtr<UVoraxiaCameraComponent> CameraComponent;
	float PanelWidth = 420.0f;

	TSharedRef<SWidget> MakeHeading(const FText& Text) const;
	TSharedRef<SWidget> MakeMetric(const FText& Label, TAttribute<FText> ValueText) const;
	TSharedRef<SWidget> MakeDivider() const;

	FText GetModeText() const;
	FText GetCollisionText() const;
	FText GetFocusActiveText() const;
	FText GetFocusHasTargetText() const;
	FText GetFocusAlphaText() const;
	FText GetFocusTargetText() const;
	FText GetFocusLocationText() const;
	FText GetScanScannableText() const;
	FText GetScanNameText() const;
	FText GetScanTimeText() const;
	FText GetScanSummaryText() const;
	FText GetScanCompositionText() const;
	FText GetDesiredDistanceText() const;
	FText GetEffectiveDistanceText() const;
	FText GetCollisionDistanceText() const;
	FText GetFOVText() const;
	FText GetDesiredRotationText() const;
	FText GetSmoothedRotationText() const;
	FText GetPivotLocationText() const;
	FText GetDesiredCameraLocationText() const;
	FText GetFinalCameraLocationText() const;
	FText GetSummaryText() const;
};