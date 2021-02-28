//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Item.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemModified);

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	IR_Common		UMETA(DisplayName = "Common"),
	IR_Uncommon		UMETA(DisplayName = "Uncommon"),
	IR_Rare			UMETA(DisplayName = "Rare"),
	IR_VeryRare		UMETA(DisplayName = "Very Rare"),
	IR_Legendary	UMETA(DisplayName = "Legendary")
};

/*Item master class. 
Every item inherits from here and it can we set it to whatever we want.*/
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class SURVIVALGAME_API UItem : public UObject
{
	GENERATED_BODY()
	
protected:

	virtual void GetLifetimeReplicatedProps( TArray<class FLifetimeProperty> & OutLifetimeProps ) const override;
	
	/* How Unreal checks if an UObject supports networking*/
	virtual bool IsSupportedForNetworking() const override;

	virtual class UWorld* GetWorld() const override;

#if WITH_EDITOR
	
	/*Allows designers to change properties only inside UnrealEngine Editor.
	When we click stackable, we can change MaxStackSize and Quantity. */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

public:

	UItem();

	UPROPERTY(Transient)
	class UWorld* World;

	/*The mesh to display for this items pickup*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	class UStaticMesh* PickupMesh;

	/*Item's thumbnail*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	class UTexture2D* Thumbnail;

	/*The display name for this item in the inventory*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText ItemDisplayName;

	/*Optional description for the item*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
	FText ItemDescription;

	/* (Equip, Eat, Consume, etc)*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText UseActionText;

	/*The item's rarity*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity;

	/*Item's weight*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0.0))
	float Weight;

	/*Whether or not this item can be stacked*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	bool bStackable;

	/*The maximum size that a stack of items can be. For example, we can 50 ammo items but only one AK-47 item.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 2, EditCondition = bStackable))
	int32 MaxStackSize;

	/*The tooltip in the inventory for this item*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	TSubclassOf<class UItemTooltip> ItemTooltip;

	/*The amount of the item. The server is gonna handle this value*/
	UPROPERTY(ReplicatedUsing = OnRep_Quantity, EditAnywhere, Category = "Item", meta = (UIMin = 1, EditCondition = bStackable))
	int32 Quantity;

	/*The inventory that owns this item*/
	UPROPERTY()
	class UInventoryComponent* OwningInventory;

	/*Used to efficiently replicate inventory items*/
	UPROPERTY()
	int32 RepKey;

	UPROPERTY(BlueprintAssignable)
	FOnItemModified OnItemModified;

public:
	
	UFUNCTION()
	void OnRep_Quantity();

	/*Sets the new quantity from Editor and replicates*/
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(const int32 NewQuantity);

	/*A blueprint pin that returns the quantity of the item*/
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE int32 GetQuantity() const { return Quantity; }

	/*Returns the weight of the stack. For example, if we have 200 ammo, it will return the final weight of all that ammo*/
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE float GetStackWeight() const { return Quantity * Weight; };

	/*There are items that we don't show in the inventory sometimes. Like if we equip something, we still have that item but we don't want to show it on inventory. */
	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool ShouldShowInInventory() const;

	UFUNCTION(BlueprintImplementableEvent)
	void OnUse(class ASurvivalCharacter* Character);

	/*What to do when we use this item*/
	virtual void Use(class ASurvivalCharacter* Character);
	virtual void AddedToInventory(class UInventoryComponent* Inventory);

	/*Mark the object as needing replication. We must call this internally after modifying any replicated properties. */
	void MarkDirtyForReplication();

};
