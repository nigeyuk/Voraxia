#include "Game/VoraxiaGameMode.h"

#include "Player/VoraxiaPlayerCharacter.h"
#include "Player/VoraxiaPlayerController.h"

AVoraxiaGameMode::AVoraxiaGameMode()
{
	PlayerControllerClass = AVoraxiaPlayerController::StaticClass();
	DefaultPawnClass = AVoraxiaPlayerCharacter::StaticClass();
}