//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/TargetPoint.h"
#include "ItemSpawn.generated.h"

//It has an array of items and the probability to spawn that item.
USTRUCT(BlueprintType)
struct FLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	//The item(s) to spawn
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<TSubclassOf<class UItem>> Items;

	//The percentage chance of spawning this item if we hit it on the roll
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = 0.001, ClampMax = 1.0))
	float Probability = 1.f;

};

/*It will spawn different items like clothes or weapons in the world.*/
UCLASS()
class SURVIVALGAME_API AItemSpawn : public ATargetPoint
{
	GENERATED_BODY()
	
public:

	AItemSpawn();

	UPROPERTY(EditAnywhere, Category = "Loot")
	class UDataTable* LootTable;

	/*Because pickups use a Blueprint base, we use a UPROPERTY to select it. */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<class APickup> PickupClass;

	/*Range used for generating re-spawn time. */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	FIntPoint RespawnRange;

protected:

	FTimerHandle TimerHandle_RespawnItem;

	UPROPERTY()
	TArray<AActor*> SpawnedPickups;

	virtual void BeginPlay() override;

	/*It will spawn an item in the world. Loaded on server.*/
	UFUNCTION()
	void SpawnItem();

	/*This is bound to the item being destroyed, so we can queue up another item to be spawned in. */
	UFUNCTION()
	void OnItemTaken(AActor* DestroyedActor);

};
