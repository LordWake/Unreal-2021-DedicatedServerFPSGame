//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/SurvivalDamageTypes.h"
#include "Weapon.generated.h"

class UAnimMontage;
class ASurvivalCharacter;
class UAudioComponent;
class UParticleSystemComponent;
class UCameraShake;
class UForceFeedbackEffect;
class USoundCue;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle, 
	Firing,
	Reloading,
	Equipping
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_USTRUCT_BODY()

	/*Clip size*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	int32 AmmoPerClip;

	/*The item that this weapon uses as ammo. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	TSubclassOf<class UAmmoItem> AmmoClass;

	/*Time between two consecutive shots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float TimeBetweenShots;

	/*Defaults*/
	FWeaponData()
	{
		AmmoPerClip = 20;
		TimeBetweenShots = 0.2f;
	}
};

USTRUCT()
struct FWeaponAnim
{
	GENERATED_USTRUCT_BODY()

	/*Animation played on pawn (1st person view).*/
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* Pawn1P;

	/*Animation played on pawn (3rd person view).*/
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* Pawn3P;
};

USTRUCT(BlueprintType)
struct FHitScanConfiguration
{
	GENERATED_BODY()


	FHitScanConfiguration()
	{
		Distance = 10000.f;
		Damage = 25.f;
		Radius = 0.f;
		DamageType = UWeaponDamage::StaticClass();
		ClientSideHitLeeway = 300.f;
	}

	/* A map of bone -> Damage amount. If the bone is a child of the given bone, it will use this damage amount.
	A value of 2 means double damage, etc. It can be used for something like head shots, or make extra damage with a special
	weapon in the legs.*/
	UPROPERTY(EditDefaultsOnly, Category = "Trace Info")
	TMap<FName, float> BoneDamageModifiers;

	/* How far the hit scan traces for a hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace Info")
	float Distance;

	/* The amount of damage to deal when we hit a player with the hit scan. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace Info")
	float Damage;

	/* Optional trace radius. A value of zero is just a line trace, anything higher is a sphere trace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace Info")
	float Radius;

	/* Client side hit leeway for BoundingBox check. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace Info")
	float ClientSideHitLeeway;

	/* Type of damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace Info")
	TSubclassOf<UDamageType> DamageType;
};

/*The weapon itself. Most of the code was copied from ShooterGame. 
This class handles the reload, the ammo, the shooting, etc.*/
UCLASS()
class SURVIVALGAME_API AWeapon : public AActor
{
	GENERATED_BODY()
	
	friend class ASurvivalCharacter;

public:	

	AWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

protected:

	/* Consume a Bullet from the ammo clip.*/
	void UseClipAmmo();
	/* Consume ammo from inventory.*/
	void ConsumeAmmo(const int32 Amount);
	/*[Server] Return ammo to the inventory when the weapon is unequipped.*/
	void ReturnAmmoToInventory();

	/* Weapon is being equipped by owner pawn.*/
	virtual void OnEquip();

	/* Weapon is now equipped by owner pawn.*/
	virtual void OnEquipFinished();

	/* Weapon is holstered by owner pawn.*/
	virtual void OnUnEquip();

	/* Check if it's currently equipped.*/
	bool IsEquipped() const;

	/* Check if mesh is already attached.*/
	bool IsAttachedToPawn() const;

	//=======================================================================
	//============================= INPUT ===================================
	//=======================================================================

	/*[Local + Server] Start weapon fire.*/
	virtual void StartFire();

	/*[Local + Server] Stop weapon fire.*/
	virtual void StopFire();

	/*[All] Start weapon reload.*/
	virtual void StartReload(bool bFromReplication = false);

	/*[Local + Server] Interrupt weapon reload.*/
	virtual void StopReload();

	/*[Server] Perform actual reload.*/
	virtual void ReloadWeapon();

	/*We only want to fire if we have ammo.*/
	bool CanFire() const;
	/*We only can reload if we have enough ammo to do the reload.*/
	bool CanReload() const;

	/*Gets weapon current state.*/
	UFUNCTION(BlueprintPure, Category = "Weapon")
	EWeaponState GetCurrentState() const;

	/*Get current ammo amount (total),*/
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const;

	/*Get current ammo amount (clip).*/
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmoInClip() const;

	/*Get clip size.*/
	int32 GetAmmoPerClip() const;

	/*Get weapon mesh (needs pawn owner to determine variant).*/
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const;

	/*Get pawn owner.*/
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	class ASurvivalCharacter* GetPawnOwner() const;

	/*Set the weapon's owning pawn.*/
	void SetPawnOwner(ASurvivalCharacter* SurvivalCharacter);

	/*Gets last time when this weapon was switched to.*/
	float GetEquipStartedTime() const;

	/*Gets the duration of equipping weapon.*/
	float GetEquipDuration() const;

protected:

	/*The weapon item in the players inventory.*/
	UPROPERTY(Replicated, BlueprintReadOnly, Transient)
	class UWeaponItem* Item;

	/*Pawn owner.*/
	UPROPERTY(Transient, ReplicatedUsing = OnRep_PawnOwner)
	class ASurvivalCharacter* PawnOwner;

	/*Weapon data.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	FWeaponData WeaponConfig;

	/*Line trace data. Will be used if projectile class is null.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	FHitScanConfiguration HitScanConfig;

public:
	
	/*The Weapon mesh.*/
	UPROPERTY(EditAnywhere, Category = Components)
	USkeletalMeshComponent* WeaponMesh;

protected:

	/*Adjustment to handle frame rate affecting actual timer interval.*/
	UPROPERTY(Transient)
	float TimerIntervalAdjustment;

	/*Whether to allow automatic weapons to catch up with shorter re-fire cycles.*/
	UPROPERTY(Config)
	bool bAllowAutomaticWeaponCatchup = true;

	/*Firing audio (bLoopedFireSound set)*/
	UPROPERTY(Transient)
	UAudioComponent* FireAC;

	/*Name of bone/socket for muzzle in weapon mesh. */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName MuzzleAttachPoint;

	/*Name of the socket to attach to the character on.*/
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName AttachSocket1P;

	/*Name of the socket to attach to the character on.*/
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName AttachSocket3P;

	/*FX for muzzle flash.*/
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* MuzzleParticles;

	/*FX for impact particles.*/
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* ImpactParticles;

	/*Spawned component for muzzle FX*/
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;

	/*Spawned component for second muzzle FX (Needed for split screen)*/
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSCSecondary;

	/*Camera shake on firing.*/
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<UCameraShake> FireCameraShake;

	/*The time it takes to aim down sights, in seconds*/
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	float ADSTime;

	/*The amount of recoil to apply. We choose a random point from 0-1 on the curve and use it to drive recoil.
	This means designers get lots of control over the recoil pattern.*/
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	class UCurveVector* RecoilCurve;

	/*The speed at which the recoil bumps up per second.*/
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilSpeed;

	/*The speed at which the recoil resets per second.*/
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilResetSpeed;

	/*Force feedback effect to play when the weapon is fired. For example, controller's vibration.*/
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UForceFeedbackEffect* FireForceFeedback;

	/* Single fire sound (bLoopedFireSound not set).*/
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireSound;

	/*Looped fire sound (bLoopedFireSound set).*/
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireLoopSound;

	/*Finished burst sound (bLoopedFireSound set).*/
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireFinishSound;

	/*Out of ammo sound.*/
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* OutOfAmmoSound;

	/*Reload sound.*/
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* ReloadSound;

	/*Reload animations.*/
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim ReloadAnim;

	/*Equip sound.*/
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* EquipSound;

	/*Equip animations.*/
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim EquipAnim;

	/*Fire animations.*/
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim FireAnim;

	/*Fire animations.*/
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim FireAimingAnim;

	/*Is muzzle FX looped?*/
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	uint32 bLoopedMuzzleFX : 1;

	/*Is fire sound looped?*/
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	uint32 bLoopedFireSound : 1;

	/*Is fire animation looped?*/
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	uint32 bLoopedFireAnim : 1;

	/*Is fire animation playing?*/
	uint32 bPlayingFireAnim : 1;

	/*Is weapon currently equipped?*/
	uint32 bIsEquipped : 1;

	/*Is weapon fire active?*/
	uint32 bWantsToFire : 1;

	/*Is reload animation playing?*/
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Reload)
	uint32 bPendingReload : 1;

	/*Is equip animation playing?*/
	uint32 bPendingEquip : 1;

	/*Weapon is re-firing, */
	uint32 bRefiring;

	/*Current weapon state.*/
	EWeaponState CurrentState;

	/*Time of last successful weapon fire.*/
	float LastFireTime;

	/*Last time when this weapon was switched to.*/
	float EquipStartedTime;

	/*How much time weapon needs to be equipped.*/
	float EquipDuration;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_HitNotify)
	FVector HitNotify;

	UFUNCTION()
	void OnRep_HitNotify();

	/*Current ammo - inside clip. */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmoInClip;

	/*Burst counter, used for replicating fire events to remote clients. */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 BurstCounter;

	/*Handle for efficient management of OnEquipFinished timer.*/
	FTimerHandle TimerHandle_OnEquipFinished;

	/* Handle for efficient management of StopReload timer.*/
	FTimerHandle TimerHandle_StopReload;

	/* Handle for efficient management of ReloadWeapon timer.*/
	FTimerHandle TimerHandle_ReloadWeapon;

	/* Handle for efficient management of HandleFiring timer.*/
	FTimerHandle TimerHandle_HandleFiring;

	//=======================================================================
	//======================= SERVER INPUTS =================================
	//=======================================================================

	/*Trigger reload from server.*/
	UFUNCTION(reliable, client)
	void ClientStartReload();

	UFUNCTION(Reliable, Server)
	void ServerStartFire();

	UFUNCTION(Reliable, Server)
	void ServerStopFire();

	UFUNCTION(Reliable, Server)
	void ServerStartReload();

	UFUNCTION(Reliable, Server)
	void ServerStopReload();

	//=======================================================================
	//===================== REPLICATION & EFFECTS ===========================
	//=======================================================================
	
	/*Called when someone takes the weapon.*/
	UFUNCTION()
	void OnRep_PawnOwner();

	/*For replicate fire FX.*/
	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();

	/*Called in network play to do the cosmetic FX for firing.*/
	virtual void SimulateWeaponFire();

	/*Called in network play to stop cosmetic FX (e.g. for a looping shot).*/
	virtual void StopSimulatingWeaponFire();

	//=======================================================================
	//===================== WEAPON USAGE ====================================
	//=======================================================================

	/*[Local] Weapon specific fire implementation.*/
	virtual void FireShot();

	/*[Server] Fire and Update ammo.*/
	UFUNCTION(Server, Reliable)
	void ServerHandleFiring();

	/*[Local + Server] Handle weapon re-fire, compensating for slack time if the timer can't sample fast enough.*/
	void HandleReFiring();

	/*[Local + Server] Handle weapon fire.*/
	void HandleFiring();

	/*[Local + Server] Firing started.*/
	virtual void OnBurstStarted();

	/*[Local + Server] Firing finished.*/
	virtual void OnBurstFinished();

	/*Update weapon state.*/
	void SetWeaponState(EWeaponState NewState);

	/*Determine current weapon state.*/
	void DetermineWeaponState();

	/*Attaches weapon mesh to pawn's mesh.*/
	void AttachMeshToPawn();

	//=======================================================================
	//===================== WEAPON USAGE HELPERS ============================
	//=======================================================================

	/*Play weapon sounds.*/
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);

	/*Play weapon animations.*/
	float PlayWeaponAnimation(const FWeaponAnim& Animation);

	/*Stop playing weapon animations.*/
	void StopWeaponAnimation(const FWeaponAnim& Animation);

	/*Get the aim of the camera.*/
	FVector GetCameraAim() const;

	/*Called in network play to do the cosmetic FX.*/
	void SimulateInstantHit(const FVector& Origin);

	/*Spawn effects for impact */
	void SpawnImpactEffects(const FHitResult& Impact);

public:

	/*Handle damage. */
	void DealDamage(const FHitResult& Impact, const FVector& ShootDir);

	/*Find hit.*/
	FHitResult WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const;

	void ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir);

	/*Server notified of hit from client to verify.*/
	UFUNCTION(Reliable, Server)
	void ServerNotifyHit(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir);

	/*Continue processing the instant hit, as if it has been confirmed by the server.*/
	void ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir);

};
