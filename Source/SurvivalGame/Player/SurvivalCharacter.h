//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SurvivalCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquippedItemsChanged, const EEquippableSlot, Slot, const UEquippableItem*, Item);

/*Stores interaction data */
USTRUCT()
struct FInteractionData
{
	GENERATED_BODY()

	FInteractionData()
	{
		ViewedInteractionComponent = nullptr;
		LastInteractionCheckTime = 0.15f;
		bInteractHeld = false;
	}

	/*The current interactable component we're viewing */
	UPROPERTY()
	class UInteractionComponent* ViewedInteractionComponent;

	/*The time when we last checked for an interactable */
	UPROPERTY()
	float LastInteractionCheckTime;

	/*Whether the local player is holding the interact key */
	UPROPERTY()
	bool bInteractHeld;
};

UCLASS()
class SURVIVALGAME_API ASurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ASurvivalCharacter();

	/*The mesh to have equipped if we don't have an item equipped.*/
	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TMap<EEquippableSlot, USkeletalMesh*> NakedMeshes;

	/*The player body meshes.*/
	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TMap<EEquippableSlot, USkeletalMeshComponent*> PlayerMeshes;

	/*Our players inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* PlayerInventory;

	/*Interaction component used to allow other players to loot us when we have died.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInteractionComponent* LootPlayerInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, Category = "Components")
	class UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* HelmetMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* ChestMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* LegsMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* FeetMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* VestMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* HandsMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* BackpackMesh;

protected:
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;
	virtual void Restart() override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void SetActorHiddenInGame(bool bNewHidden) override;

public:

	/*Takes an inventory and starts to loot from that.*/
	UFUNCTION(BlueprintCallable)
	void SetLootSource(class UInventoryComponent* NewLootSource);

	/*Returns true if the loot source is not a nullptr.*/
	UFUNCTION(BlueprintPure, Category = "Looting")
	bool IsLooting() const;

protected:

	/*The inventory that we are currently looting from. */
	UPROPERTY(ReplicatedUsing = OnRep_LootSource, BlueprintReadOnly)
	UInventoryComponent* LootSource;

	/*Begin being looted by a player*/
	UFUNCTION()
	void BeginLootingPlayer(class ASurvivalCharacter* Character);

	/*Sets the loot source to null.*/
	UFUNCTION()
	void OnLootSourceOwnerDestroyed(AActor* DestroyedActor);

	/*The server needs to be given the loot source.*/
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSetLootSource(class UInventoryComponent* NewLootSource);

	/*Shows the loot source we are interacting with.*/
	UFUNCTION()
	void OnRep_LootSource();

public:

	/* Information about current state of the players interaction */
	UPROPERTY()
	FInteractionData InteractionData;

	FTimerHandle TimerHandle_Interact;

	/* How often in seconds to check for an interactable object */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckFrequency;

	/* How far we'll trace when we check if the player is looking at an interactable object */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InterationCheckDistance;

	/*Takes the item.*/
	UFUNCTION(BlueprintCallable, Category = "Looting")
	void LootItem(class UItem* ItemToGive);

	UFUNCTION(Server, Reliable)
	void ServerLootItem(class UItem* ItemToLoot);

	/* Makes a LineTrace on the Tick function to check if it can find or not an interactable object */
	void PerformInteractionCheck();
	/* Clear the timer, stops all interactions and clear the old InteractionComponent in case we already had one */
	void CouldntFindInteractable();
	/* Saves the new interactable and start to focus this one */
	void FoundNewInteractable(UInteractionComponent* Interactable);
	/*Calls the interaction component to do something on interact, and it checks if it must hold the key button or just press*/
	void BeginInteract();
	/*Clear timer and if it has an InteractionComponent saved, it will call the interactable EndInteract function*/
	void EndInteract();

	UFUNCTION(Server, Reliable)
	void ServerBeginInteract();
	UFUNCTION(Server, Reliable)
	void ServerEndInteract();

	/*Tells the interactable component that we interacting with it.*/
	void Interact();

	FORCEINLINE class UInteractionComponent* GetInteractable() const { return InteractionData.ViewedInteractionComponent; }

public:

	/*We need this because the pickups use a blueprint base class. */
	UPROPERTY(EditDefaultsOnly, Category = "Items")
	TSubclassOf<class APickup> PickupClass;

	/* True if we're interacting with an item that has an interaction time (for example a lamp that takes 2 seconds to turn on) */
	bool IsInteracting() const;

	/* Get the time till we interact with the current interactable */
	float GetRemainingInteractTime() const;

	/*[Server] Use an item from out inventory*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void UseItem(class UItem* Item);

	UFUNCTION(Server, Reliable)
	void ServerUseItem(class UItem* Item);

	/*[Server] Drop an item*/
	UFUNCTION(BlueprintCallable, Category = "Items")
	void DropItem(class UItem* Item, const int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerDropItem(class UItem* Item, const int32 Quantity);

public:

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UPROPERTY(BlueprintAssignable, Category = "Items")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

	/*Add the item to EquippedItems map and executes OnEquippedItemsChanged delegate.*/
	bool EquipItem(class UEquippableItem* Item);
	/*Remove the item to EquippedItems map and executes OnEquippedItemsChanged delegate.*/
	bool UnEquipItem(class UEquippableItem* Item);

	/*Sets the new Mesh and Material when equip a gear.*/
	void EquipGear(class UGearItem* Gear);
	/*Removes the Mesh and Material in this particular slot. Return the mesh value to naked or null.*/
	void UnEquipGear(const EEquippableSlot Slot);	
	/*Spawns and equips the weapon that we are taking or choosing from our inventory.*/
	void EquipWeapon(class UWeaponItem* WeaponItem);
	/*Removes the weapon and sets EquippedWeapon to a nullptr.*/
	void UnEquipWeapon();

	/*If I have a weapon on, this will replicate that to all the players in the game.*/
	UFUNCTION()
	void OnRep_EquippedWeapon();

	/*It will return an SkeletalMeshComponent by using a EquippableSlot as a parameter.*/
	UFUNCTION(BlueprintPure)
	class USkeletalMeshComponent* GetSlotSkeletalMeshComponent(const EEquippableSlot Slot);

	/*Returns all the items that we have equipped.*/
	UFUNCTION(BlueprintPure)
	FORCEINLINE TMap<EEquippableSlot, UEquippableItem*> GetEquippedItems() const { return EquippedItems; }

	/*Returns the weapon that we have equipped.*/
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	FORCEINLINE class AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

protected:

	/*Just calls UseThrowable on the server.*/
	UFUNCTION(Server, Reliable)
	void ServerUseThrowable();

	/*Replicates the throw animation montage to other player.*/
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayThrowableTossFX(class UAnimMontage* MontageToPlay);

	/*Return the item that we have already equipped to throw.*/
	class UThrowableItem* GetThrowable() const;
	
	/*Called when you press the throw key.*/
	void UseThrowable();
	/*Spawn item in the game world.*/
	void SpawnThrowable();

	bool CanUseThrowable() const;
	
protected:
	
	//Allows for efficient access of equipped items by the slot.
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TMap<EEquippableSlot, UEquippableItem*> EquippedItems;

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Health")
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth;

public:

	/*Modify the players health by either a negative or positive amount. Return the amount of health actually removed.*/
	float ModifyHealth(const float Delta);

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthModified(const float HealthDelta);

protected:

	UPROPERTY()
	float LastMeleeAttackTime;

	UPROPERTY(EditDefaultsOnly, Category = Melee)
	float MeleeAttackDistance;

	UPROPERTY(EditDefaultsOnly, Category = Melee)
	float MeleeAttackDamage;

	UPROPERTY(EditDefaultsOnly, Category = Melee)
	class UAnimMontage* MeleeAttackMontage;

	/*If it has a weapon it will tell to that weapon to shoot. Otherwise it will call BeginMeleeAttack.*/
	void StartFire();
	/*If it has a weapon, it will tell to that weapon to stop shooting.*/
	void StopFire();
	/*Handles all the logic about attack attack and collision channels.*/
	void BeginMeleeAttack();

	/*Checks that everything is all right and applies point damage.*/
	UFUNCTION(Server, Reliable)
	void ServerProcessMeleeHit(const FHitResult& MeleeHit);

	/*Replicates attack animation.*/
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayMeleeFX();

protected:

	/*This is gonna be our killer and it will be replicated to everyone else in the game.*/
	UPROPERTY(ReplicatedUsing = OnRep_Killer)
	class ASurvivalCharacter* Killer;

	/*Called when killed by the player, or killed by something else like the environment.*/
	void Suicide(struct FDamageEvent const& DamageEvent, const AActor* DamageCauser);
	/*Called when someone else kill us.*/
	void KilledByPlayer(struct FDamageEvent const& DamageEvent, class ASurvivalCharacter* Character, const AActor* DamageCauser);

	/*Called when someone kills you.*/
	UFUNCTION()
	void OnRep_Killer();

	UFUNCTION(BlueprintImplementableEvent)
	void OnDeath();

public:
	
	UPROPERTY(EditDefaultsOnly, Category = Movement)
	float SprintSpeed;

	UPROPERTY()
	float WalkSpeed;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Movement)
	bool bSprinting;

	/*Returns true if we are not aiming.*/
	bool CanSprint() const;

	/*[Local] Starts sprinting function.*/
	void StartSprinting();
	/*[Local] Stops sprinting function.*/
	void StopSprinting();

	/*[Server + Local] Set Sprinting.*/
	void SetSprinting(const bool bNewSprinting);

	/*Calls SetSprinting on the server.*/
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(const bool bNewSprinting);

	/*Calls the Crouch function from Character.*/
	void StartCrouching();
	/*Calls the UnCrouch function from Character.*/
	void StopCrouching();

	void MoveFoward(float Val);
	void MoveRight(float Val);
	void LookUp(float Val);
	void Turn(float Val);

protected:

	UPROPERTY(Transient, Replicated)
	bool bIsAiming;

	/*Returns true if we have an equipped weapon.*/
	bool CanAim() const;

	/*If CanAim(), calls SetAiming with a true value.*/
	void StartAiming();
	/*Calls SetAiming with a false value.*/
	void StopAiming();
	/*Sets bIsAiming value.*/
	void SetAiming(const bool bNewAiming);

	/*Calls SetAiming on the server.*/
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(const bool bNewAiming);

public:

	/*If it has an equipped weapon, it will that weapon to reload.*/
	void StartReload();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsAlive() const { return Killer == nullptr; };

	UFUNCTION(BlueprintPure, Category = "Weapons")
	FORCEINLINE bool IsAiming() const { return bIsAiming; }
};
