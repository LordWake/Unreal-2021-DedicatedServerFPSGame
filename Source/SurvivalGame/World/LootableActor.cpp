//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "LootableActor.h"

#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/DataTable.h"
#include "World/ItemSpawn.h"

#include "Items/Item.h"

#include "Player/SurvivalCharacter.h"

#define LOCTEXT_NAMESPACE "LootableActor"

ALootableActor::ALootableActor()
{
	LootContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>("LootContainerMesh");
	SetRootComponent(LootContainerMesh);

	LootInteraction = CreateDefaultSubobject<UInteractionComponent>("LootInteraction");
	LootInteraction->InteractableActionText = LOCTEXT("LootActorText", "Loot");
	LootInteraction->InteractableNameText	= LOCTEXT("LootActorName", "Chest");
	LootInteraction->SetupAttachment(GetRootComponent());

	Inventory = CreateDefaultSubobject<UInventoryComponent>("Inventory");
	Inventory->SetCapacity(20);
	Inventory->SetWeightCapacity(80.f);

	LootRolls = FIntPoint(2, 8); 

	SetReplicates(true);
}

void ALootableActor::BeginPlay()
{
	Super::BeginPlay();
	LootInteraction->OnInteract.AddDynamic(this, &ALootableActor::OnInteract);

	//If we are the server and we have a LootTable to find.
	if (GetLocalRole() == ROLE_Authority && LootTable)
	{
		TArray<FLootTableRow*> SpawnItems;
		LootTable->GetAllRows("", SpawnItems); //Get all of the rows of that table.

		int32 Rolls = FMath::RandRange(LootRolls.GetMin(), LootRolls.GetMax()); //Get a random number between the min and max of the loot rows. 

		//Loop over that "many times" we have in the random number before
		for (int32 i = 0; i < Rolls; ++i)
		{
			const FLootTableRow* LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)]; //Get the table row.

			ensure(LootRow);

			float ProbabilityRoll = FMath::FRandRange(0.f, 1.f); //Random number between 0 a 1. Based on this number, it will decide if give or not the item.

			while (ProbabilityRoll > LootRow->Probability)
			{
				LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];
				ProbabilityRoll = FMath::FRandRange(0.f, 1.f);
			}


			if (LootRow && LootRow->Items.Num())
			{
				for (auto& ItemClass : LootRow->Items)
				{
					if (ItemClass)
					{
						//If the Item is valid, we get the default quantity of that item
						const int32 Quantity = Cast<UItem>(ItemClass->GetDefaultObject())->GetQuantity();
						//And add it to the inventory of this actor. Like for example, a chest.
						Inventory->TryAddItemFromClass(ItemClass, Quantity);
					}
				}
			}
		}
	}
}

void ALootableActor::OnInteract(class ASurvivalCharacter* Character)
{
	if (Character)
	{
		Character->SetLootSource(Inventory);
	}
}

#undef LOCTEXT_NAMESPACE
