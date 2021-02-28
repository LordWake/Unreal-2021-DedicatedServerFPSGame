//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryItemWidget.generated.h"

/*The item reference inside the inventory.*/
UCLASS()
class SURVIVALGAME_API UInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	/*Reference to the item it has to display. ExposedOnSpawn will create a pin to this value*/
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item Widget", meta = (ExposeOnSpawn = true))
	class UItem* Item;

};
