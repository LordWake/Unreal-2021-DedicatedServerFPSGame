//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "SurvivalCharacter.h"
#include "SurvivalGame.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/DamageType.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"

#include "Items/EquippableItem.h"
#include "Items/GearItem.h"
#include "Items/WeaponItem.h"
#include "Items/ThrowableItem.h"

#include "Weapons/SurvivalDamageTypes.h"
#include "Weapons/Weapon.h"
#include "Weapons/ThrowableWeapon.h"

#include "World/Pickup.h"

#include "Net/UnrealNetwork.h"
#include "Player/SurvivalPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstance.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "SurvivalCharacter"
static FName NAME_AimDownSightsSocket("ADSSocket");

ASurvivalCharacter::ASurvivalCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("SpringArmComponent");
	SpringArmComponent->SetupAttachment(GetMesh(), FName("CameraSocket"));
	SpringArmComponent->TargetArmLength = 0.f;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = true;

	HelmetMesh		= PlayerMeshes.Add(EEquippableSlot::EIS_Helmet,		CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HelmetMesh")));
	ChestMesh		= PlayerMeshes.Add(EEquippableSlot::EIS_Chest,		CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh")));
	LegsMesh		= PlayerMeshes.Add(EEquippableSlot::EIS_Legs,		CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh")));
	FeetMesh		= PlayerMeshes.Add(EEquippableSlot::EIS_Feet,		CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh")));
	VestMesh		= PlayerMeshes.Add(EEquippableSlot::EIS_Vest,		CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VestMesh")));
	HandsMesh		= PlayerMeshes.Add(EEquippableSlot::EIS_Hands,		CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandsMesh")));
	BackpackMesh	= PlayerMeshes.Add(EEquippableSlot::EIS_Backpack,	CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BackpackMesh")));

	//Tell all the body meshes to use the head for mesh animations.
	for (auto& PlayerMesh : PlayerMeshes)
	{
		USkeletalMeshComponent* MeshComponent = PlayerMesh.Value;
		MeshComponent->SetupAttachment(GetMesh());
		MeshComponent->SetMasterPoseComponent(GetMesh()); //This is used to make all the body parts follow the head blueprint animation.
	}

	//We don't want to attach the head to head, so we add it to playerMeshes after the for loop.
	PlayerMeshes.Add(EEquippableSlot::EIS_Head, GetMesh());

	PlayerInventory = CreateDefaultSubobject<UInventoryComponent>("PlayerInventory");
	PlayerInventory->SetCapacity(20);
	PlayerInventory->SetWeightCapacity(80.f);

	LootPlayerInteraction = CreateDefaultSubobject<UInteractionComponent>("PlayerInteraction");
	LootPlayerInteraction->InteractableActionText	= LOCTEXT("LootPlayerText", "Loot");
	LootPlayerInteraction->InteractableNameText		= LOCTEXT("LootPlayerName", "Player");
	LootPlayerInteraction->SetupAttachment(GetRootComponent());
	LootPlayerInteraction->SetActive(false, true);
	LootPlayerInteraction->bAutoActivate = false;

	InteractionCheckFrequency	= 0.f;
	InterationCheckDistance		= 1000.f;

	MaxHealth = 100.f;
	Health = MaxHealth;

	SprintSpeed = GetCharacterMovement()->MaxWalkSpeed * 1.5f;
	WalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	bIsAiming = false;

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetMesh()->SetOwnerNoSee(true);

	SetReplicateMovement(true);
	SetReplicates(true);
	bAlwaysRelevant = true;
}

void ASurvivalCharacter::BeginPlay()
{
	Super::BeginPlay();

	LootPlayerInteraction->OnInteract.AddDynamic(this, &ASurvivalCharacter::BeginLootingPlayer);

	//Try to display the players platform name on their loot card.
	if (APlayerState* PS = GetPlayerState())
	{
		LootPlayerInteraction->SetInteractableNameText(FText::FromString(PS->GetPlayerName()));
	}

	//When the player spawns they have no items equipped, so save that items (that way, if a player unEquips an item we can set the mesh back to one of these).
	for (auto& PlayerMesh : PlayerMeshes)
	{
		NakedMeshes.Add(PlayerMesh.Key, PlayerMesh.Value->SkeletalMesh);
	}
}

void ASurvivalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//Action
	PlayerInputComponent->BindAction("Fire",		IE_Pressed,  this,	&ASurvivalCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire",		IE_Released, this,	&ASurvivalCharacter::StopFire);

	PlayerInputComponent->BindAction("Throw",		IE_Pressed,  this,	&ASurvivalCharacter::UseThrowable);
	
	PlayerInputComponent->BindAction("Aim",			IE_Pressed,  this,	&ASurvivalCharacter::StartAiming);
	PlayerInputComponent->BindAction("Aim",			IE_Released, this,	&ASurvivalCharacter::StopAiming);

	PlayerInputComponent->BindAction("Jump",		IE_Pressed,  this,	&ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump",		IE_Released, this,	&ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Interact",	IE_Pressed,  this,	&ASurvivalCharacter::BeginInteract);
	PlayerInputComponent->BindAction("Interact",	IE_Released, this,	&ASurvivalCharacter::EndInteract);

	PlayerInputComponent->BindAction("Crouch",		IE_Pressed,  this,	&ASurvivalCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch",		IE_Released, this,	&ASurvivalCharacter::StopCrouching);
	
	PlayerInputComponent->BindAction("Sprint",		IE_Pressed,  this,	&ASurvivalCharacter::StartSprinting);
	PlayerInputComponent->BindAction("Sprint",		IE_Released, this,	&ASurvivalCharacter::StopSprinting);

	//Axis
	PlayerInputComponent->BindAxis("MoveForward",	this,	&ASurvivalCharacter::MoveFoward);
	PlayerInputComponent->BindAxis("MoveRight",		this,	&ASurvivalCharacter::MoveRight);
}

void ASurvivalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASurvivalCharacter, bSprinting);
	DOREPLIFETIME(ASurvivalCharacter, Killer);
	DOREPLIFETIME(ASurvivalCharacter, EquippedWeapon);

	DOREPLIFETIME_CONDITION(ASurvivalCharacter, LootSource, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASurvivalCharacter, Health,		COND_OwnerOnly); //Only the owner needs to know about the health. But the server can be authoritative.
	DOREPLIFETIME_CONDITION(ASurvivalCharacter, bIsAiming,	COND_SkipOwner); //It will replicate aiming to everyone else in the game (to make animations) except the owner.
}

void ASurvivalCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//If the time since the last check is higher that the frequency we set, then execute this functions. This way, we can make a little bit of optimization and don't run this function every frame on the server.
	if (GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	{
		PerformInteractionCheck();
	}

	if (IsLocallyControlled())
	{
		//Changes FOV if we are aiming.
		const float DesiredFOV = IsAiming() ? 70.f : 100.f;
		CameraComponent->SetFieldOfView(FMath::FInterpTo(CameraComponent->FieldOfView, DesiredFOV, DeltaTime, 10.f));

		if (EquippedWeapon)
		{
			//Get Aim Down Sights location.
			const FVector ADSLocation = EquippedWeapon->GetWeaponMesh()->GetSocketLocation(NAME_AimDownSightsSocket);
			//Get Camera's Location.
			const FVector DefaultCameraLocation = GetMesh()->GetSocketLocation(FName("CameraSocket"));

			const FVector CameraLoc = bIsAiming ? ADSLocation : DefaultCameraLocation;

			const float InterpSpeed = FVector::Dist(ADSLocation, DefaultCameraLocation) / EquippedWeapon->ADSTime;
			
			//Interpolate camera to new Location.
			CameraComponent->SetWorldLocation(FMath::VInterpTo(CameraComponent->GetComponentLocation(), CameraLoc, DeltaTime, InterpSpeed));
		}
	}
}

void ASurvivalCharacter::Restart()
{
	Super::Restart();

	if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
	{
		PC->ShowInGameUI(); //Called every time the player restarts. 
	}
}

float ASurvivalCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	const float DamageDealt = ModifyHealth(-Damage);

	if (Health <= 0.f)
	{
		if (ASurvivalCharacter* KillerCharacter = Cast<ASurvivalCharacter>(DamageCauser->GetOwner()))
		{
			KilledByPlayer(DamageEvent, KillerCharacter, DamageCauser);
		}
		else
		{
			Suicide(DamageEvent, DamageCauser);
		}
	}

	return DamageDealt;
}

void ASurvivalCharacter::SetActorHiddenInGame(bool bNewHidden)
{
	Super::SetActorHiddenInGame(bNewHidden);

	//If we have an equipped weapon, we'll want to hide it too when we get inside the car.
	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(bNewHidden);
	}
}

#pragma region Loot

void ASurvivalCharacter::SetLootSource(class UInventoryComponent* NewLootSource)
{
	//If the item we're looting gets destroyed, we need to tell the client to remove their Loot Screen.
	if (NewLootSource && NewLootSource->GetOwner())
	{
		NewLootSource->GetOwner()->OnDestroyed.AddUniqueDynamic(this, &ASurvivalCharacter::OnLootSourceOwnerDestroyed);
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		if (NewLootSource)
		{
			if (ASurvivalCharacter* Character = Cast<ASurvivalCharacter>(NewLootSource->GetOwner()))
			{
				Character->SetLifeSpan(120.f); //If we are looting from this dead character, keep it alive another 2 minutes so we can loot.
			}
		}

		LootSource = NewLootSource;
		OnRep_LootSource();
	}
	else
	{
		ServerSetLootSource(NewLootSource);
	}
}

bool ASurvivalCharacter::IsLooting() const
{
	return LootSource != nullptr; //We are looting if we have a LootSource. So if it isn't equal to null ptr, returns true.
}

void ASurvivalCharacter::BeginLootingPlayer(class ASurvivalCharacter* Character)
{
	if (Character)
	{
		Character->SetLootSource(PlayerInventory);
	}
}

void ASurvivalCharacter::ServerSetLootSource_Implementation(class UInventoryComponent* NewLootSource)
{
	SetLootSource(NewLootSource);
}

void ASurvivalCharacter::OnLootSourceOwnerDestroyed(AActor* DestroyedActor)
{
	//Remove loot source.
	if (GetLocalRole() == ROLE_Authority && LootSource && DestroyedActor == LootSource->GetOwner())
	{
		ServerSetLootSource(nullptr);
	}
}

void ASurvivalCharacter::OnRep_LootSource()
{
	//Bring up or remove the looting menu
	if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
	{
		if (PC->IsLocalController()) //Only do this if it is locally controlled.
		{
			if (LootSource)
			{
				PC->ShowLootMenu(LootSource);
			}
			else
			{
				PC->HideLootMenu();
			}
		}
	}
}

void ASurvivalCharacter::LootItem(class UItem* ItemToGive)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		if (PlayerInventory && LootSource && ItemToGive && LootSource->HasItem(ItemToGive->GetClass(), ItemToGive->GetQuantity()))
		{
			const FItemAddResult AddResult = PlayerInventory->TryAddItem(ItemToGive);

			if (AddResult.ActualAmountGiven > 0)
			{
				LootSource->ConsumeItem(ItemToGive, AddResult.ActualAmountGiven);
			}
			else //If for some reason you can't take the item...
			{
				if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
				{
					PC->ClientShowNotification(AddResult.ErrorText); //Tell the player why they couldn't loot the item.
				}
			}
		}
	}
	else
	{
		ServerLootItem(ItemToGive);
	}
}

void ASurvivalCharacter::ServerLootItem_Implementation(class UItem* ItemToLoot)
{
	LootItem(ItemToLoot);
}

#pragma endregion

#pragma region Interactable components

void ASurvivalCharacter::PerformInteractionCheck()
{
	if (GetController() == nullptr)
	{
		return;
	}

	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	FVector EyesLocation;
	FRotator EyesRotation;

	GetController()->GetPlayerViewPoint(EyesLocation, EyesRotation);

	FVector TraceStart = EyesLocation;
	FVector TraceEnd = (EyesRotation.Vector() * InterationCheckDistance) + TraceStart;

	FHitResult TraceHit;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		if (TraceHit.GetActor())
		{
			//Does this actor contains this UInteractionComponent class?
			if (UInteractionComponent* InteractionComponent = Cast<UInteractionComponent>(TraceHit.GetActor()->GetComponentByClass(UInteractionComponent::StaticClass())))
			{
				//How far away are we from the object.
				float Distance = (TraceStart - TraceHit.ImpactPoint).Size();

				//Check if we can interact with this object if we are close enough
				if (InteractionComponent != GetInteractable() && Distance <= InteractionComponent->InteractionDistance)
				{
					FoundNewInteractable(InteractionComponent);
				}

				//If we are too far...
				else if (Distance > InteractionComponent->InteractionDistance && GetInteractable())
				{
					CouldntFindInteractable();
				}

				return; //At this point, we already know if we find or not an interactable so return and break the function.
			}
		}
	}

	CouldntFindInteractable();
}

void ASurvivalCharacter::CouldntFindInteractable()
{
	//We've lost focus on an interactable. Clear the timer
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Interact))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interact);
	}

	//Tell the interactable we've stopped focusing on it, and clear the current interactable
	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndFocus(this);

		if (InteractionData.bInteractHeld)
			EndInteract();
	}

	//Clear the interaction component data.
	InteractionData.ViewedInteractionComponent = nullptr;
}

void ASurvivalCharacter::FoundNewInteractable(UInteractionComponent* Interactable)
{
	// If we found a new interactable, stop interacting with the last one 
	EndInteract();

	//Stop focus the last interactable
	if (UInteractionComponent* OldInteractable = GetInteractable())
	{
		OldInteractable->EndFocus(this);
	}

	//Focus the new one and save it
	InteractionData.ViewedInteractionComponent = Interactable;
	Interactable->BeginFocus(this);
}

void ASurvivalCharacter::BeginInteract()
{
	//If we are not the server, ask the server to execute this
	if (GetLocalRole() != ROLE_Authority)
	{
		ServerBeginInteract();
	}

	/**As an optimization, the server only checks that we're looking at an item once we begin interacting with it.
	This saves the server doing a check every tick for an interactable Item. The exception is a non-instant interact.
	In this case, the server will check every tick for the duration of the interact*/
	if (GetLocalRole() == ROLE_Authority)
	{
		PerformInteractionCheck();
	}

	InteractionData.bInteractHeld = true;

	//Do we have an interaction component?
	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->BeginInteract(this);

		//If we don't need to hold a key, just interact
		if (FMath::IsNearlyZero(Interactable->InteractionTime))
		{
			Interact();
		}
		else
		{
			//If we need to hold a key, we'll interact after the time that this interactable needs. False to don't loop
			GetWorldTimerManager().SetTimer(TimerHandle_Interact, this, &ASurvivalCharacter::Interact, Interactable->InteractionTime, false);
		}
	}
}

void ASurvivalCharacter::EndInteract()
{
	if (GetLocalRole() != ROLE_Authority)
		ServerEndInteract();

	InteractionData.bInteractHeld = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndInteract(this);
	}
}

void ASurvivalCharacter::ServerBeginInteract_Implementation()
{
	BeginInteract();
}

void ASurvivalCharacter::ServerEndInteract_Implementation()
{
	EndInteract();
}

void ASurvivalCharacter::Interact()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->Interact(this);
	}
}

bool ASurvivalCharacter::IsInteracting() const
{
	//If this is active, it means we are interacting with something.
	return GetWorldTimerManager().IsTimerActive(TimerHandle_Interact);
}

float ASurvivalCharacter::GetRemainingInteractTime() const
{
	//Returns how much time remains on this timer
	return GetWorldTimerManager().GetTimerRemaining(TimerHandle_Interact);
}

#pragma endregion

#pragma region Use/Drop Items

void ASurvivalCharacter::UseItem(class UItem* Item)
{
	//This way we can check if we are a client and not the server.
	if (GetLocalRole() < ROLE_Authority && Item)
	{
		ServerUseItem(Item);
	}

	//If the server calls this function
	if (GetLocalRole() == ROLE_Authority)
	{
		if (PlayerInventory && !PlayerInventory->FindItem(Item))
		{
			return; //We can't use items that we don't have.
		}
	}

	//Client and server call this function.
	if (Item)
	{
		Item->OnUse(this);
		Item->Use(this);
	}
}

void ASurvivalCharacter::ServerUseItem_Implementation(class UItem* Item)
{
	UseItem(Item);
}

void ASurvivalCharacter::DropItem(class UItem* Item, const int32 Quantity)
{
	if (PlayerInventory && Item && PlayerInventory->FindItem(Item))
	{
		if (GetLocalRole() < ROLE_Authority)
		{
			ServerDropItem(Item, Quantity);
			return; //If we are not the server return. We don't want clients to spawn items.
		}

		if (GetLocalRole() == ROLE_Authority)
		{
			const int32 ItemQuantity = Item->GetQuantity(); //Store the quantity of the item.
			const int32 DroppedQuantity = PlayerInventory->ConsumeItem(Item, Quantity); //How much we have drop.

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.bNoFail = true; //Always spawn the item. 

			//If we somehow drop an item inside of a wall, Unreal is gonna try to correct that item so is not inside of the wall.
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			FVector SpawnLocation = GetActorLocation();
			//Adjust the z by half of the height. Like the actor location is the center of the player, subtract the half of the capsule and we have the feet.
			SpawnLocation.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

			ensure(PickupClass);

			APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParams);

			Pickup->InitializePickup(Item->GetClass(), DroppedQuantity); //Give the pickup the item to represent and the amount.
		}
	}
}

void ASurvivalCharacter::ServerDropItem_Implementation(class UItem* Item, const int32 Quantity)
{
	DropItem(Item, Quantity);
}

#pragma endregion

#pragma region Equip/Unequip Items, Weapons & Gear

bool ASurvivalCharacter::EquipItem(class UEquippableItem* Item)
{
	EquippedItems.Add(Item->Slot, Item);
	OnEquippedItemsChanged.Broadcast(Item->Slot, Item);
	return true;
}

bool ASurvivalCharacter::UnEquipItem(class UEquippableItem* Item)
{
	if (Item)
	{
		if (EquippedItems.Contains(Item->Slot))
		{
			if (Item == *EquippedItems.Find(Item->Slot))
			{
				EquippedItems.Remove(Item->Slot);
				OnEquippedItemsChanged.Broadcast(Item->Slot, nullptr);
				return true;
			}
		}
	}

	return false;
}

void ASurvivalCharacter::EquipGear(class UGearItem* Gear)
{
	//Which one of the skeletalMeshComponents, is this gear for?
	if (USkeletalMeshComponent* GearMesh = *PlayerMeshes.Find(Gear->Slot))
	{
		GearMesh->SetSkeletalMesh(Gear->Mesh);
		GearMesh->SetMaterial(GearMesh->GetMaterials().Num() - 1, Gear->MaterialInstance);
	}
}

void ASurvivalCharacter::UnEquipGear(const EEquippableSlot Slot)
{
	if (USkeletalMeshComponent* EquippableMesh = *PlayerMeshes.Find(Slot)) //Find this mesh
	{
		if (USkeletalMesh* BodyMesh = *NakedMeshes.Find(Slot)) //Find the naked mesh version of that slot
		{
			EquippableMesh->SetSkeletalMesh(BodyMesh); //Set that mesh to the naked one.

			//Put the materials back on the body mesh (Since gear may have applied a different material).
			for (int32 i = 0; i < BodyMesh->Materials.Num(); ++i)
			{
				if (BodyMesh->Materials.IsValidIndex(i))
				{
					EquippableMesh->SetMaterial(i, BodyMesh->Materials[i].MaterialInterface);
				}
			}
		}

		else
		{
			//For some gears like backpacks, there is no naked mesh. So we set it to nullptr.
			EquippableMesh->SetSkeletalMesh(nullptr);
		}
	}
}

void ASurvivalCharacter::EquipWeapon(class UWeaponItem* WeaponItem)
{
	if (WeaponItem && WeaponItem->WeaponClass && GetLocalRole() == ROLE_Authority)
	{
		if (EquippedWeapon) //If we already had a weapon, unequipt it.
		{
			UnEquipWeapon();
		}

		//Spawn the weapon in.
		FActorSpawnParameters SpawnParams;
		SpawnParams.bNoFail = true;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = SpawnParams.Instigator = this; //Owner and instigator is my character.

		if (AWeapon* Weapon = GetWorld()->SpawnActor<AWeapon>(WeaponItem->WeaponClass, SpawnParams))
		{
			Weapon->Item = WeaponItem;

			EquippedWeapon = Weapon;
			OnRep_EquippedWeapon();

			Weapon->OnEquip();
		}
	}
}

void ASurvivalCharacter::UnEquipWeapon()
{
	if (GetLocalRole() == ROLE_Authority && EquippedWeapon)
	{
		EquippedWeapon->OnUnEquip();
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;

		OnRep_EquippedWeapon();
	}
}

void ASurvivalCharacter::OnRep_EquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->OnEquip();
	}
}

class USkeletalMeshComponent* ASurvivalCharacter::GetSlotSkeletalMeshComponent(const EEquippableSlot Slot)
{
	if (PlayerMeshes.Contains(Slot))
	{
		return *PlayerMeshes.Find(Slot);
	}
	return nullptr;
}

#pragma endregion

#pragma region Trow Items

void ASurvivalCharacter::ServerUseThrowable_Implementation()
{
	UseThrowable();
}

void ASurvivalCharacter::MulticastPlayThrowableTossFX_Implementation(UAnimMontage* MontageToPlay)
{
	//Local player already instantly played grenade throw animation.
	if (GetNetMode() != NM_DedicatedServer && !IsLocallyControlled())
	{
		PlayAnimMontage(MontageToPlay);
	}
}

class UThrowableItem* ASurvivalCharacter::GetThrowable() const
{
	UThrowableItem* EquippedThrowable = nullptr;

	if (EquippedItems.Contains(EEquippableSlot::EIS_Throwable))
	{
		EquippedThrowable = Cast<UThrowableItem>(*EquippedItems.Find(EEquippableSlot::EIS_Throwable));
	}

	return EquippedThrowable;
}

void ASurvivalCharacter::UseThrowable()
{
	if (CanUseThrowable())
	{
		if (UThrowableItem* Throwable = GetThrowable())
		{
			if (GetLocalRole() == ROLE_Authority)
			{
				SpawnThrowable();

				if (PlayerInventory)
				{
					PlayerInventory->ConsumeItem(Throwable, 1);
				}
			}
			else
			{
				if (Throwable->GetQuantity() <= 1)
				{
					EquippedItems.Remove(EEquippableSlot::EIS_Throwable);
					OnEquippedItemsChanged.Broadcast(EEquippableSlot::EIS_Throwable, nullptr);
				}

				//Locally play grenade throw instantly - by the time server spawns the grenade in the throw animation should roughly sync up with the spawning of the grenade
				PlayAnimMontage(Throwable->ThrowableTossAnimation);
				ServerUseThrowable();
			}
		}
	}
}

void ASurvivalCharacter::SpawnThrowable()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		if (UThrowableItem* CurrentThrowable = GetThrowable())
		{
			if (CurrentThrowable->ThrowableClass)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = SpawnParams.Instigator = this;
				SpawnParams.bNoFail = true;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				FVector EyesLoc;
				FRotator EyesRot;

				GetController()->GetPlayerViewPoint(EyesLoc, EyesRot);

				//Spawn item slightly in front of our face so it doesn't collide with our player.
				EyesLoc = (EyesRot.Vector() * 20.f) + EyesLoc;

				if (AThrowableWeapon* ThrowableWeapon = GetWorld()->SpawnActor<AThrowableWeapon>(CurrentThrowable->ThrowableClass, FTransform(EyesRot, EyesLoc), SpawnParams))
				{
					MulticastPlayThrowableTossFX(CurrentThrowable->ThrowableTossAnimation);
				}
			}
		}
	}
}

bool ASurvivalCharacter::CanUseThrowable() const
{
	return GetThrowable() != nullptr && GetThrowable()->ThrowableClass != nullptr;
}

#pragma endregion

#pragma region Health Managers

float ASurvivalCharacter::ModifyHealth(const float Delta)
{
	const float OldHealth = Health;

	Health = FMath::Clamp<float>(Health + Delta, 0.f, MaxHealth);

	return Health - OldHealth;
}

void ASurvivalCharacter::OnRep_Health(float OldHealth)
{
	OnHealthModified(Health - OldHealth);
}

#pragma endregion

#pragma region Fire & Melee Attack

void ASurvivalCharacter::StartFire()
{
	if (EquippedWeapon && !bSprinting)
	{
		EquippedWeapon->StartFire();
	}
	else
	{
		BeginMeleeAttack();
	}
}

void ASurvivalCharacter::StopFire()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StopFire();
	}
}

void ASurvivalCharacter::BeginMeleeAttack()
{
	if (bSprinting)
	{
		return;
	}
	
	//Is the time since we make an attack higher than the Anim montage length?
	if(GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength())
	{
		FHitResult Hit;
		FCollisionShape Shape = FCollisionShape::MakeSphere(15.f);

		FVector StartTrace = CameraComponent->GetComponentLocation();
		FVector EndTrace = (CameraComponent->GetComponentRotation().Vector() * MeleeAttackDistance) + StartTrace;

		FCollisionQueryParams QueryParams = FCollisionQueryParams("MeleeSweep", false, this);

		PlayAnimMontage(MeleeAttackMontage);

		if (GetWorld()->SweepSingleByChannel(Hit, StartTrace, EndTrace, FQuat(), COLLISION_WEAPON, Shape, QueryParams))
		{
			UE_LOG(LogTemp, Warning, TEXT("IT WORKS!"));

			if (ASurvivalCharacter* HitPlayer = Cast<ASurvivalCharacter>(Hit.GetActor()))
			{
				if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
				{
					PC->OnHitPlayer();
				}
			}
		}

		//We tell the server that we hit someone, so he decides what to do.
		ServerProcessMeleeHit(Hit);

		LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
	}
}

void ASurvivalCharacter::ServerProcessMeleeHit_Implementation(const FHitResult& MeleeHit)
{
	if (GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength() && (GetActorLocation() - MeleeHit.ImpactPoint).Size() <= MeleeAttackDistance)
	{
		MulticastPlayMeleeFX(); //Tell everyone else in the game to play my punch animation so they can see me.

		UGameplayStatics::ApplyPointDamage(MeleeHit.GetActor(), MeleeAttackDamage, (MeleeHit.TraceStart - MeleeHit.TraceEnd).GetSafeNormal(), MeleeHit, GetController(), this, UMeleeDamage::StaticClass());
	}

	LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
}

void ASurvivalCharacter::MulticastPlayMeleeFX_Implementation()
{
	if (!IsLocallyControlled())
	{
		PlayAnimMontage(MeleeAttackMontage);
	}
}

#pragma endregion

#pragma region Kills

void ASurvivalCharacter::Suicide(struct FDamageEvent const& DamageEvent, const AActor* DamageCauser)
{
	Killer = this; //If we kill ourself, we are our own killer.
	OnRep_Killer();
}

void ASurvivalCharacter::KilledByPlayer(struct FDamageEvent const& DamageEvent, class ASurvivalCharacter* Character, const AActor* DamageCauser)
{
	Killer = Character;
	OnRep_Killer();
}

void ASurvivalCharacter::OnRep_Killer()
{
	SetLifeSpan(20.f); //After 20 seconds, we are gonna be removed from the world.

	//Turn on the mesh collisions, and make rag doll physics.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GetMesh()->SetOwnerNoSee(false);

	//Turn the capsule collisions off.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	SetReplicateMovement(false);

	//Activate LootInteraciontComponent so other players can loot from us.
	LootPlayerInteraction->Activate();

	TArray<UEquippableItem*> EquippedInvItems;
	EquippedItems.GenerateValueArray(EquippedInvItems);

	//UnEquip all equipped items so other players can loot.
	for (auto& Equippable : EquippedInvItems)
	{
		Equippable->SetEquipped(false);
	}

	//Only for ourself, this will not be replicated to anybody else.
	if (IsLocallyControlled())
	{
		SpringArmComponent->TargetArmLength = 500.f;
		SpringArmComponent->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		bUseControllerRotationPitch = true; //Unlock camera's rotation around the character.

		if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
		{
			PC->ShowDeathScreen(Killer);
		}
	}
}

#pragma  endregion

#pragma region Movement, Crouch & Sprint

bool ASurvivalCharacter::CanSprint() const
{
	return !IsAiming();
}

void ASurvivalCharacter::StartSprinting()
{
	SetSprinting(true);
}

void ASurvivalCharacter::StopSprinting()
{
	SetSprinting(false);
}

void ASurvivalCharacter::SetSprinting(const bool bNewSprinting)
{
	if ((bNewSprinting && !CanSprint()) || bNewSprinting == bSprinting)
	{
		return;
	}

	if (GetLocalRole() != ROLE_Authority)
	{
		ServerSetSprinting(bNewSprinting);
	}

	bSprinting = bNewSprinting;

	GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
}

void ASurvivalCharacter::ServerSetSprinting_Implementation(const bool bNewSprinting)
{
	SetSprinting(bNewSprinting);
}

void ASurvivalCharacter::StartCrouching()
{
	Crouch();
}

void ASurvivalCharacter::StopCrouching()
{
	UnCrouch();
}

void ASurvivalCharacter::MoveFoward(float Val)
{
	if (Val != 0.f)
		AddMovementInput(GetActorForwardVector(), Val);
}

void ASurvivalCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
		AddMovementInput(GetActorRightVector(), Val);
}

void ASurvivalCharacter::LookUp(float Val)
{
	if (Val != 0.f)
		AddControllerPitchInput(Val);
}

void ASurvivalCharacter::Turn(float Val)
{
	if (Val != 0.f)
		AddControllerYawInput(Val);
}

#pragma endregion

#pragma region Aim & Reload

bool ASurvivalCharacter::CanAim() const
{
	return EquippedWeapon != nullptr;
}

void ASurvivalCharacter::StartAiming()
{
	if (CanAim())
	{
		SetAiming(true);
	}
}

void ASurvivalCharacter::StopAiming()
{
	SetAiming(false);
}

void ASurvivalCharacter::SetAiming(const bool bNewAiming)
{
	//If you can't aim, or you're trying to set aiming to what already is, return.
	if ((bNewAiming && !CanAim()) || bNewAiming == bIsAiming)
	{
		return;
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetAiming(bNewAiming);
	}

	bIsAiming = bNewAiming;
}

void ASurvivalCharacter::ServerSetAiming_Implementation(const bool bNewAiming)
{
	SetAiming(bNewAiming);
}

void ASurvivalCharacter::StartReload()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StartReload();
	}
}

#pragma endregion

#undef  LOCTEXT_NAMESPACE