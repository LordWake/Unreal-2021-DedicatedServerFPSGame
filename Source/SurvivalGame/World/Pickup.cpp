//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "Pickup.h"

#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#include "Player/SurvivalCharacter.h"

#include "Components/StaticMeshComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"

#include "Items/Item.h"

APickup::APickup()
{
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>("PickupMesh");
	PickupMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); //Pawn can't collide with pickup mesh

	SetRootComponent(PickupMesh);

	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>("PickupInteractionComponent");
	InteractionComponent->InteractionTime = 0.5f;
	InteractionComponent->InteractionDistance = 200.f;
	InteractionComponent->InteractableNameText = FText::FromString("Pickup");
	InteractionComponent->InteractableActionText = FText::FromString("Take");
	InteractionComponent->OnInteract.AddDynamic(this, &APickup::OnTakePickup);
	InteractionComponent->SetupAttachment(PickupMesh);

	SetReplicates(true);
}

void APickup::InitializePickup(const TSubclassOf<class UItem> ItemClass, const int32 Quantity)
{
	if (GetLocalRole() == ROLE_Authority && ItemClass && Quantity > 0)
	{
		Item = NewObject<UItem>(this, ItemClass);
		Item->SetQuantity(Quantity);

		OnRep_Item(); //So clients can update the item
		Item->MarkDirtyForReplication();
	}
}

void APickup::OnRep_Item()
{
	if (Item)
	{
		PickupMesh->SetStaticMesh(Item->PickupMesh);

		//Change the pickup name on world UI
		InteractionComponent->InteractableNameText = Item->ItemDisplayName;

		//Clients bind to this delegate in order to refresh the interaction widget if item quantity changes.
		Item->OnItemModified.AddDynamic(this, &APickup::OnItemModified);
	}

	//If any replicated properties on the item are changed, we refresh the widget
	InteractionComponent->RefreshWidget();
}

void APickup::OnItemModified()
{
	if (InteractionComponent)
	{
		InteractionComponent->RefreshWidget();
	}
}

void APickup::BeginPlay()
{
	Super::BeginPlay();
	
	//bNetStartup is true if the pickup is placed/spawned on the level, and false if someone drop it.
	if (GetLocalRole() == ROLE_Authority && ItemTemplate && bNetStartup)
	{
		InitializePickup(ItemTemplate->GetClass(), ItemTemplate->GetQuantity());
	}

	/*If pickup was spawned at runtime, ensure that it matches the rotation of the ground that it was dropped on
	If we dropped a pickup on a 20 degree slope, the pickup would also be spawned at a 20 degree angle.*/
	if (!bNetStartup)
	{
		AlignWithGround();
	}

	if (Item)
	{
		Item->MarkDirtyForReplication();
	}
}

void APickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APickup, Item);
}

bool APickup::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	//Does the item needs to be replicate?
	if (Item && Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
	{
		//Replicate it 
		bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
	}			
	
	return bWroteSomething;
}

#if WITH_EDITOR
void APickup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//If a new pickup is selected in the property editor, change the mesh to reflect the new item being selected
	if (PropertyName == GET_MEMBER_NAME_CHECKED(APickup, ItemTemplate))
	{
		if (ItemTemplate)
		{
			PickupMesh->SetStaticMesh(ItemTemplate->PickupMesh);
		}
	}
}
#endif

void APickup::OnTakePickup(class ASurvivalCharacter* Taker)
{
	if (!Taker)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pickup was taken but player was not valid."));
		return;
	}

	//Pending kill to prevent the player from taking a pickup another player has already tried taking.
	if (GetLocalRole() == ROLE_Authority && !IsPendingKill() && Item)
	{
		if (UInventoryComponent* PlayerInventory = Taker->PlayerInventory)
		{
			const FItemAddResult AddResult = PlayerInventory->TryAddItem(Item);

			//If we don't take all of the items in the pickup
			if (AddResult.ActualAmountGiven < Item->GetQuantity())
			{
				//Do not destroy the pickup, just increase the item amount.
				Item->SetQuantity(Item->GetQuantity() - AddResult.ActualAmountGiven);
			}

			//If we did take all of the items
			else if (AddResult.ActualAmountGiven >= Item->GetQuantity())
			{
				Destroy(); //Destroy the pickup, we already take all the items amount
			}
		}
	}
}