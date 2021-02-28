//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTooltip.generated.h"

UCLASS()
class SURVIVALGAME_API UItemTooltip : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip", meta = (ExposeOnSpawn = true))
	class UInventoryItemWidget* InventoryItemWidget;

};
