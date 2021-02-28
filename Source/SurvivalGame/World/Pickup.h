//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

/*An actor represented in the world that holds an item.*/
UCLASS()
class SURVIVALGAME_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	

	APickup();

	/*Takes the item to represent and creates the pickup from it. Done on BeginPlay and when a player drops an item on the ground.*/
	void InitializePickup(const TSubclassOf<class UItem> ItemClass, const int32 Quantity);

	/*Align pickups rotation with ground rotation. */
	UFUNCTION(BlueprintImplementableEvent)
	void AlignWithGround();

	/* This is used as a template to create the pickup when spawned in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	class UItem* ItemTemplate;

protected:
	
	/*The item that will be added to the inventory when this pickup is taken. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, ReplicatedUsing = OnRep_Item)
	class UItem* Item;

	UFUNCTION()
	void OnRep_Item();

	/*If some property on the item is modified, we bind this to OnItemModified and refresh the UI if the item gets modified.*/
	UFUNCTION()
	void OnItemModified();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/*Called when a player takes the pickup*/
	UFUNCTION()
	void OnTakePickup(class ASurvivalCharacter* Taker);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractionComponent* InteractionComponent;
};
