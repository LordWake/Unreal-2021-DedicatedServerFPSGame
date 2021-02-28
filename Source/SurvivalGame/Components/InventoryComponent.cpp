//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h" //Need it to replicate UObjects

#define LOCTEXT_NAMESPACE "Inventory"

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

FItemAddResult UInventoryComponent::TryAddItem(class UItem* Item)
{
	return TryAddItem_Internal(Item);
}

FItemAddResult UInventoryComponent::TryAddItemFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity)
{
	//Creates an item
	UItem* Item = NewObject<UItem>(GetOwner(), ItemClass);
	//Sets the Item quantity
	Item->SetQuantity(Quantity);
	return TryAddItem_Internal(Item);
}

int32 UInventoryComponent::ConsumeItem(class UItem* Item)
{
	if (Item)
	{
		ConsumeItem(Item, Item->GetQuantity());
	}

	return 0;
}

int32 UInventoryComponent::ConsumeItem(class UItem* Item, const int32 Quantity)
{
	//HasAuthory to check if it is the server. Only the server can consume items.
	if (Item && GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity()); //How much of the items to consume
		
		//We shouldn't have a negative amount of the item after the drop
		ensure((!Item->GetQuantity() - RemoveQuantity < 0));

		//Set the Item quantity to the new quantity
		Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

		//If it reach zero, remove this item from the inventory. There is no point if we have "0" guns.
		if (Item->GetQuantity() <= 0)
		{
			RemoveItem(Item);
		}
		else
		{
			//If we had 5 items and remove 2, we need to tell the client to refresh their inventory.
			ClientRefreshInventory();
		}

		return RemoveQuantity;
	}

	return 0;
}

bool UInventoryComponent::RemoveItem(class UItem* Item)
{
	//HasAuthory to check if it is the server. Only the server can remove items.
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		if (Item)
		{
			Items.RemoveSingle(Item);

			OnRep_Items();

			//If we don't update this, the server is not gonna update the items to the clients
			ReplicatedItemsKey++;
			return true;
		}
	}

	return false;
}

bool UInventoryComponent::HasItem(TSubclassOf<class UItem> ItemClass, const int32 Quantity) const
{
	if (UItem* ItemToFind = FindItemByClass(ItemClass))
	{
		return ItemToFind->GetQuantity() >= Quantity; //Checks if the items has the required quantity.
	}

	return false;
}

UItem* UInventoryComponent::FindItem(class UItem* Item) const
{
	if (Item)
	{
		for (auto& InventoryItem : Items)
		{
			if (InventoryItem && InventoryItem->GetClass() == Item->GetClass())
			{
				return InventoryItem;
			}
		}
	}

	return nullptr;
}

UItem* UInventoryComponent::FindItemByClass(TSubclassOf<class UItem> ItemClass) const
{
	for (auto& InventoryItem : Items)
	{
		if (InventoryItem && InventoryItem->GetClass() == ItemClass)
		{
			return InventoryItem;
		}
	}
	return nullptr;
}

TArray<UItem*> UInventoryComponent::FindAllItemsByClass(TSubclassOf<class UItem> ItemClass) const
{
	TArray<UItem*> ItemsOfClass; 

	for (auto& InventoryItem : Items)
	{
		if (InventoryItem && InventoryItem->GetClass()->IsChildOf(ItemClass))
		{
			ItemsOfClass.Add(InventoryItem);
		}
	}

	return ItemsOfClass;
}

float UInventoryComponent::GetCurrentWeight() const
{
	float Weight = 0.f;

	for (auto& Item : Items)
	{
		if (Item)
		{
			Weight += Item->GetStackWeight();
		}
	}

	return Weight;
}

void UInventoryComponent::SetWeightCapacity(const float NewWeightCapacity)
{
	WeightCapacity = NewWeightCapacity;
	OnInventoryUpdated.Broadcast(); //Update UI
}

void UInventoryComponent::SetCapacity(const int32 NewCapacity)
{
	Capacity = NewCapacity;
	OnInventoryUpdated.Broadcast(); //Update UI
}

void UInventoryComponent::ClientRefreshInventory_Implementation()
{
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UInventoryComponent, Items);
}

bool UInventoryComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	//Whether or not we wrote something in the actor channel
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	//Check if the array of items needs to replicate
	if (Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey))
	{
		for (auto& Item : Items)
		{
			//Go through every item and check if it needs to replicate
			if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
			{
				//bWroteSomething = bWroteSomething OR | Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
				bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
			}
		}
	}

	return bWroteSomething;

}

UItem* UInventoryComponent::AddItem(class UItem* Item)
{
	//If we have an owner and that owner is on the server. Only the server can add items
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		//Like we don't know if this InventoryComponent is the owner of the item, we re-construct it.
		//Make a NewItem, get the class and tell that our owner is it owner.
		UItem* NewItem = NewObject<UItem>(GetOwner(), Item->GetClass());
		
		NewItem->World = GetWorld();
		NewItem->SetQuantity(Item->GetQuantity());
		NewItem->OwningInventory = this;
		NewItem->AddedToInventory(this);
		
		Items.Add(NewItem);
		NewItem->MarkDirtyForReplication(); //This will use the RepKey to replicates the Item
		
		return NewItem;
	}

	return nullptr;
}

void UInventoryComponent::OnRep_Items()
{
	OnInventoryUpdated.Broadcast();
	
	for (auto& Item : Items)
	{
		//On the client the world won't be set initially, so it set if not.
		if (!Item->World)
		{
			Item->World = GetWorld();
		}
	}
}

FItemAddResult UInventoryComponent::TryAddItem_Internal(class UItem* Item)
{
	//If we are in the server...
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		const int32 AddAmount = Item->GetQuantity(); //Amount that we're trying to add
		
		//We can't add items if the inventory is full
		if (Items.Num() + 1 > GetCapacity())
		{
			return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryCapacityFullText", "Couldn't add item to inventory. Inventory is full."));
		}

		//Items with a weight of zero don't require a weight check.
		if (!FMath::IsNearlyZero(Item->Weight))
		{
			//If we are gonna exceed the weight capacity...
			if (GetCurrentWeight() + Item->Weight > GetWeightCapacity())
			{
				return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryTooMuchWeightText", "Couldn't add item to inventory. Carrying too much weigh."));
			}
		}

		//If the item is stackable, check if we already have it and add it to their stack
		if (Item->bStackable)
		{
			//Somehow the items quantity went over the max stack size. This shouldn't ever happen.
			ensure(Item->GetQuantity() <= Item->MaxStackSize);

			//If we have this item in our items, it doesn't make sense to add a new item. 
			//We just increase the item's quantity in our inventory.
			if (UItem* ExistingItem = FindItem(Item))
			{
				if (ExistingItem->GetQuantity() < ExistingItem->MaxStackSize)
				{
					//Find out how much of the item we can add
					const int32 CapacityMaxAddAmount = ExistingItem->MaxStackSize - ExistingItem->GetQuantity();
					int32 ActualAddAmount = FMath::Min(AddAmount, CapacityMaxAddAmount);

					FText ErrorText = LOCTEXT("InventoryErrorText", "Couldn't add all of the items to your inventory");

					//If the weight of the item is not nearly zero...
					if (!FMath::IsNearlyZero(Item->Weight))
					{
						//Find the max amount of the item we could take due to weight
						const int32 WeightMaxAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->Weight);						
						ActualAddAmount = FMath::Min(ActualAddAmount, WeightMaxAmount);

						if (ActualAddAmount < AddAmount)
						{
							ErrorText = FText::Format(LOCTEXT("InventoryTooMuchWeightText", "Couldn't add entire stack of {ItemName} to Inventory"), Item->ItemDisplayName);
						}
					}
					else if (ActualAddAmount < AddAmount)
					{
						//If the item weights none and we can take it, then there was a capacity issue.
						ErrorText = FText::Format(LOCTEXT("InventoryCapacityFullText", "Couldn't add entire stack of {ItemName} to Inventory. Inventory was full."), Item->ItemDisplayName);
					}

					//We couldn't add any of the items to the inventory
					if (ActualAddAmount <= 0)
					{
						return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryErrorText", "Couldn't add item to the inventory."));
					}

					ExistingItem->SetQuantity(ExistingItem->GetQuantity() + ActualAddAmount); //Set the new quantity to the item if everything goes fine!

					//If we somehow get more of the item than the max stack size, then something is wrong with our math.
					ensure(ExistingItem->GetQuantity() <= ExistingItem->MaxStackSize);

					//If we couldn't add ALL of the items...
					if (ActualAddAmount < AddAmount)
					{
						return FItemAddResult::AddedSome(AddAmount, ActualAddAmount, ErrorText);
					}
					else
					{
						return FItemAddResult::AddedAll(AddAmount);
					}
				}
				
				else
				{
					return FItemAddResult::AddedNone(AddAmount, FText::Format(LOCTEXT("InventoryFullStackText", "Couldn't add {ItemName}. You already have a full stack of this item"), Item->ItemDisplayName));
				}
			}

			//If we don't have this stackable item in our inventory
			else
			{
				AddItem(Item);
				return FItemAddResult::AddedAll(Item->Quantity);
			}

		}
		
		//If the item isn't stackable
		else 
		{
			if (Items.Num() + 1 > GetCapacity())
			{
				return FItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(LOCTEXT("InventoryCapacityFullText", "Couldn't add {ItemName} to Inventory. Inventory is full."), Item->ItemDisplayName));
			}

			//Items with a weight of zero don't require a weight check
			if (!FMath::IsNearlyZero(Item->Weight))
			{
				if (GetCurrentWeight() + Item->Weight > GetWeightCapacity())
				{
					return FItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(LOCTEXT("StackWeightFullText", "Couldn't add {ItemName}, too much weight."), Item->ItemDisplayName));
				}
			}

			//Non-stackable should always have a quantity of 1
			ensure(Item->GetQuantity() == 1);

			AddItem(Item);
			return FItemAddResult::AddedAll(Item->GetQuantity());
		}
	}

	//AddItem should never be called on a client.
	check(false);
	return FItemAddResult::AddedNone(-1, LOCTEXT("ErrorMessage", ""));
}

#undef LOCTEXT_NAMESPACE