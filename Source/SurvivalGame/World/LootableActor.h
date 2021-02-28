//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootableActor.generated.h"

/*An actor we some loot inside the inventory. The chest inherits from here.*/
UCLASS()
class SURVIVALGAME_API ALootableActor : public AActor
{
	GENERATED_BODY()
	
public:	

	ALootableActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* LootContainerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInteractionComponent* LootInteraction;

	/**The items in the Lootable actor are held in here*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UDataTable* LootTable;

	//The number of times to roll the loot table. Random number between min and max will be used. 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	FIntPoint LootRolls;

protected:
	
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInteract(class ASurvivalCharacter* Character);
};
