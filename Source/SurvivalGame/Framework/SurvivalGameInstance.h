//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SurvivalGameInstance.generated.h"


UCLASS()
class SURVIVALGAME_API USurvivalGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnQuitToDesktop();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnQuitToMenu();
};
