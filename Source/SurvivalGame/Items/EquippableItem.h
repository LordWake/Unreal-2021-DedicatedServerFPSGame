//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "EquippableItem.generated.h"

//All the slots that gear can be equipped to. EIS = equip Item Slot.
UENUM(BlueprintType)
enum class EEquippableSlot : uint8
{
	EIS_Head			UMETA(DisplayName = "Head"),
	EIS_Helmet			UMETA(DisplayName = "Helmet"),
	EIS_Chest			UMETA(DisplayName = "Chest"),
	EIS_Vest			UMETA(DisplayName = "Vest"),
	EIS_Legs			UMETA(DisplayName = "Legs"),
	EIS_Feet			UMETA(DisplayName = "Feet"),
	EIS_Hands			UMETA(DisplayName = "Hands"),
	EIS_Backpack		UMETA(DisplayName = "Backpack"),
	EIS_PrimaryWeapon	UMETA(DisplayName = "Primary Weapon"),
	EIS_Throwable		UMETA(DisplayName = "Throwable Item")
};


/* Everything that can be equipped in the character, it will be handle from this class.
 We can't make a blueprint from this Item, we need to make a child. Like GearItem. */
UCLASS(Abstract, NotBlueprintable)
class SURVIVALGAME_API UEquippableItem : public UItem
{
	GENERATED_BODY()
	
public:

	UEquippableItem();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippables")
	EEquippableSlot Slot;

	virtual void GetLifetimeReplicatedProps( TArray<class FLifetimeProperty> & OutLifetimeProps) const override;	

	/*Checks if it has to equip or unEquip the item when we want to use it.*/
	virtual void Use(class ASurvivalCharacter* Character) override;

	/*It will equipped the item.*/
	UFUNCTION(BlueprintCallable, Category = "Equippables")
	virtual bool Equip(class ASurvivalCharacter* Character);

	/*It will UnEquip the item.*/
	UFUNCTION(BlueprintCallable, Category = "Equippables")
	virtual bool UnEquip(class ASurvivalCharacter* Character);

	/*If the items is equipped, it will be hidden in inventory.*/
	virtual bool ShouldShowInInventory() const override;

	void AddedToInventory(class UInventoryComponent* Inventory) override;

	UFUNCTION(BlueprintPure, Category = "Equippables")
	bool IsEquipped() { return bEquipped; };

	/*Changes bEquipped status and calls EquipStatusChanged in the server and all of the clients*/
	void SetEquipped(bool bNewEquipped);

protected:

	/* It'll be true if the item is equipped and false it isn't. */
	UPROPERTY(ReplicatedUsing = EquipStatusChanged)
	bool bEquipped;

	/*Checks if it have to equip or unEquip the item depending of bEquipped.*/
	UFUNCTION()
	void EquipStatusChanged();

};
