//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "Item.h"
#include "Components/InventoryComponent.h"
#include "Net/UnrealNetwork.h"

#define  LOCTEXT_NAMESPACE "Item"

void UItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/*Tells the Server to handle this value. Now is replicated*/
	DOREPLIFETIME(UItem, Quantity); 
}

bool UItem::IsSupportedForNetworking() const
{
	return true;
}

class UWorld* UItem::GetWorld() const
{
	return World;
}

#if WITH_EDITOR
void UItem::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//UPROPERTY Clamping doesn't support using a variable to clamp so we do in here instead.
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UItem, Quantity))
	{
		//Change the property in editor and then clamp it. For example, if we say that
		//Quantity is equal to 10 but MaxStackSize is 5, it will clamp it to 5 in Editor.
		Quantity = FMath::Clamp(Quantity, 1, bStackable ? MaxStackSize : 1);
	}
}
#endif

UItem::UItem()
{
	ItemDisplayName = LOCTEXT("ItemName", "Item");
	UseActionText	= LOCTEXT("ItemUseActionText", "Use");
	
	Weight			= 0.f;
	bStackable		= false;
	Quantity		= 1;
	MaxStackSize	= 2;
	RepKey			= 0;
}

void UItem::OnRep_Quantity()
{
	OnItemModified.Broadcast();
}

void UItem::SetQuantity(const int32 NewQuantity)
{
	if (NewQuantity != Quantity)
	{
		//Sets the new quantity if this is a Stackable type.
		Quantity = FMath::Clamp(NewQuantity, 0, bStackable ? MaxStackSize : 1);
		MarkDirtyForReplication();
	}
}

bool UItem::ShouldShowInInventory() const
{
	return true;
}

void UItem::Use(class ASurvivalCharacter* Character)
{

}

void UItem::AddedToInventory(class UInventoryComponent* Inventory)
{

}

void UItem::MarkDirtyForReplication()
{
	//Mark this object for replication
	++RepKey;

	//Mark the array for replication
	if (OwningInventory)
	{
		++OwningInventory->ReplicatedItemsKey;
	}
}

#undef LOCTEXT_NAMESPACE