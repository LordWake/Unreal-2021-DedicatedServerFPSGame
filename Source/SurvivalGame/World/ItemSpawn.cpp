//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "ItemSpawn.h"
#include "World/Pickup.h"
#include "Items/Item.h"

AItemSpawn::AItemSpawn()
{
	PrimaryActorTick.bCanEverTick = false;
	bNetLoadOnClient = false; //It will only get loaded on the server. 

	RespawnRange = FIntPoint(10, 30);
}

void AItemSpawn::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		SpawnItem();
	}
}

void AItemSpawn::SpawnItem()
{
	if (GetLocalRole() == ROLE_Authority && LootTable)
	{
		TArray<FLootTableRow*> SpawnItems;
		LootTable->GetAllRows("", SpawnItems);

		const FLootTableRow* LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];

		ensure(LootRow);

		float ProbabilityRoll = FMath::FRandRange(0.f, 1.f);

		while (ProbabilityRoll > LootRow->Probability)
		{
			LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];
			ProbabilityRoll = FMath::FRandRange(0.f, 1.f);
		}


		if (LootRow && LootRow->Items.Num() && PickupClass)
		{
			float Angle = 0.f;

			for (auto& ItemClass : LootRow->Items)
			{
				//Offset from where the item should be spawn, make it spawn it inside a circle.
				const FVector LocationOffset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 50.f;

				//Spawn Parameters.
				FActorSpawnParameters SpawnParams;
				SpawnParams.bNoFail = true;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				//Spawn quantity.
				const int32 ItemQuantity = ItemClass->GetDefaultObject<UItem>()->GetQuantity();

				FTransform SpawnTransform = GetActorTransform();
				SpawnTransform.AddToTranslation(LocationOffset); //Add an offset to the spawnPosition.
				
				APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParams); //Spawn the pickup with the PickupClass, the SpawnTransform with the offset and the SpawnParams.				
				Pickup->InitializePickup(ItemClass, ItemQuantity); //Initialize pickUp Item with class and quantity (example, AK-47, 1).
				Pickup->OnDestroyed.AddUniqueDynamic(this, &AItemSpawn::OnItemTaken); //Bind it to OnDestroyed so we can know when this item was taken.

				SpawnedPickups.Add(Pickup);

				//Increment the angle to make a nice circle effect between all the spawned items.
				Angle += (PI * 2.f) / LootRow->Items.Num();
			}
		}
	}
}

void AItemSpawn::OnItemTaken(AActor* DestroyedActor)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		SpawnedPickups.Remove(DestroyedActor);

		//If all pickups were taken queue a re-spawn
		if (SpawnedPickups.Num() <= 0)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_RespawnItem, this, &AItemSpawn::SpawnItem, FMath::RandRange(RespawnRange.GetMin(), RespawnRange.GetMax()), false);
		}
	}
}
