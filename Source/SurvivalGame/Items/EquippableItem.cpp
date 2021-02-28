//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "EquippableItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/SurvivalCharacter.h"
#include "Components/InventoryComponent.h"

#define  LOCTEXT_NAMESPACE "EquippableItem"

UEquippableItem::UEquippableItem()
{
	bStackable = false;
	bEquipped = false;
	UseActionText = LOCTEXT("ItemUseActionText", "Equip");
}

void UEquippableItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquippableItem, bEquipped);
}

void UEquippableItem::Use(class ASurvivalCharacter* Character)
{
	if (Character && Character->GetLocalRole() == ROLE_Authority)
	{
		//If the character contains that item in their slots, and is not equipped.
		if (Character->GetEquippedItems().Contains(Slot) && !bEquipped)
		{			
			UEquippableItem* AlreadyEquippedItem = *Character->GetEquippedItems().Find(Slot); //We find that item inside the characters Items			
			AlreadyEquippedItem->SetEquipped(false); //And change that item state
		}

		//If the item is equipped, it will unEquip.
		//If the item is unequipped, equip it.
		SetEquipped(!IsEquipped());
	}
}

bool UEquippableItem::Equip(class ASurvivalCharacter* Character)
{
	if (Character)
	{
		return Character->EquipItem(this);
	}
	return false;
}

bool UEquippableItem::UnEquip(class ASurvivalCharacter* Character)
{
	if (Character)
	{
		return Character->UnEquipItem(this);
	}
	return false;
}

bool UEquippableItem::ShouldShowInInventory() const
{
	return !bEquipped;
}

void UEquippableItem::AddedToInventory(class UInventoryComponent* Inventory)
{
	//If the player looted an item don't equip it
	if (ASurvivalCharacter* Character = Cast<ASurvivalCharacter>(Inventory->GetOwner()))
	{
		//Don't equip if we are looting.
		if (Character && !Character->IsLooting())
		{
			//If we take an item that we can equip, and don't have an item equipped at its slot, them auto equip it.
			if (!Character->GetEquippedItems().Contains(Slot))
			{
				SetEquipped(true);
			}
		}
	}
}

void UEquippableItem::SetEquipped(bool bNewEquipped)
{
	bEquipped = bNewEquipped;
	EquipStatusChanged(); //Call this function in the server and all of the clients.
	MarkDirtyForReplication();
}

void UEquippableItem::EquipStatusChanged()
{
	if (ASurvivalCharacter* Character = Cast<ASurvivalCharacter>(GetOuter()))
	{
		UseActionText = bEquipped ? LOCTEXT("UnequipText", "UnEquip") : LOCTEXT("EquipText", "Equip");
		 
		if (bEquipped)
		{
			Equip(Character);
		}
		else
		{
			UnEquip(Character);
		}

		//Tell the UI to Update
		OnItemModified.Broadcast();
	}
}

#undef LOCTEXT_NAMESPACE
