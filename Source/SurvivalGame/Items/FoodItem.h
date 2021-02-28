//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "FoodItem.generated.h"

/* Food item in the game world, usually, it will heal us.*/
UCLASS()
class SURVIVALGAME_API UFoodItem : public UItem
{
	GENERATED_BODY()
	
public:
		
	UFoodItem();

	/*The amount for the food to heal*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Healing")
	float HealAmount;

	virtual void Use(class ASurvivalCharacter* Character) override;

};
