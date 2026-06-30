// Copyright Coding Custard Studios

/**
 * @file SVoraxiaTerrainProbeWidget.h
 * @brief Slate overlay widget for the local Voraxia terrain probe readout.
 */

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UVoraxiaPlanetDebugComponent;

/**
 * @brief Displays live local terrain-probe data in the PIE game viewport.
 *
 * The widget owns no generation state. It reads formatted diagnostic text from
 * the planet debug component, allowing it to remain visible when F8 ejects the
 * possessed pawn and the editor free camera takes control.
 */
class SVoraxiaTerrainProbeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVoraxiaTerrainProbeWidget)
	{
	}

		/**
		 * @brief Debug component that supplies the live terrain-probe readout.
		 */
		SLATE_ARGUMENT(
			TWeakObjectPtr<UVoraxiaPlanetDebugComponent>,
			DebugComponent)

	SLATE_END_ARGS()

	/**
	 * @brief Constructs the compact terrain-probe viewport overlay.
	 *
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs);

private:
	/**
	 * @brief Returns the latest terrain-probe text.
	 *
	 * @return Current multi-line diagnostic readout.
	 */
	FText GetProbeText() const;

	/**
	 * @brief Returns whether the overlay should currently be visible.
	 *
	 * @return Visible while its source component has debug presentation enabled.
	 */
	EVisibility GetProbeVisibility() const;

	/**
	 * @brief Weak source of terrain-probe state.
	 */
	TWeakObjectPtr<UVoraxiaPlanetDebugComponent> DebugComponent;
};
