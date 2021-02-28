//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionWidget.generated.h"

/*Interaction card world widget.*/
UCLASS()
class SURVIVALGAME_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (ExposeOnSpawn))
	class UInteractionComponent* OwningInteractionComponent;
	
public:

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateInteractionWidget(class UInteractionComponent* InteractionComponent);

	UFUNCTION(BlueprintImplementableEvent)
	void OnUpdateInteractionWidget();	
};
