//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Items/EquippableItem.h"
#include "ThrowableItem.generated.h"

/*Every item that we can throw, like grenades, inherits from here.*/
UCLASS(Blueprintable)
class SURVIVALGAME_API UThrowableItem : public UEquippableItem
{
	GENERATED_BODY()
	
public:

	/*The montage to play when we throw an item.*/
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	class UAnimMontage* ThrowableTossAnimation;

	/*The actor to spawn in when we throw the item.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class AThrowableWeapon> ThrowableClass;

};
