#include "Game/VoraxiaGameMode.h"

#include "VoraxiaLogCategories.h"
#include "Player/VoraxiaPlayerCharacter.h"
#include "Player/VoraxiaPlayerController.h"

AVoraxiaGameMode::AVoraxiaGameMode()
{
	{
		UE_LOG(LogVoraxiaGame, Log, TEXT("Voraxia game mode active."));
	}
	
	PlayerControllerClass = AVoraxiaPlayerController::StaticClass();
	DefaultPawnClass = AVoraxiaPlayerCharacter::StaticClass();
}