//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalPlayerController.generated.h"


UCLASS()
class SURVIVALGAME_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ASurvivalPlayerController();

	virtual void SetupInputComponent() override;

	/*Call this instead of ShowNotification if on the server.*/
	UFUNCTION(Client, Reliable, BlueprintCallable)
	void ClientShowNotification(const FText& Message);
	
	UFUNCTION(BlueprintImplementableEvent)
	void ShowNotification(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowDeathScreen(class ASurvivalCharacter* Killer);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowLootMenu(const class UInventoryComponent* LootSource);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowInGameUI();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HideLootMenu();

	UFUNCTION(BlueprintImplementableEvent)
	void OnHitPlayer();

	UFUNCTION(BlueprintCallable, Category = "Survival Controller")
	void Respawn();

	UFUNCTION(Server, Reliable)
	void ServerRespawn();

	/**Applies recoil to the camera.
	@param RecoilAmount the amount to recoil by. X is the yaw, Y is the pitch
	@param RecoilSpeed the speed to bump the camera up per second from the recoil
	@param RecoilResetSpeed the speed the camera will return to center at per second after the recoil is finished*/	
	void ApplyRecoil(const FVector2D& RecoilAmount, const float RecoilSpeed, const float RecoilResetSpeed);

	/*The amount of recoil to apply. We store this in a variable as we smoothly apply the recoil over several frames.*/
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilBumpAmount;

	/*The amount of recoil the gun has had, that we need to reset (After shooting we slowly want the recoil to return to normal).*/
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilResetAmount;

	/*The speed at which the recoil bumps up per second.*/
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilSpeed;

	/*The speed at which the recoil resets per second.*/
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilResetSpeed;

	/*The last time that we applied recoil.*/
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	float LastRecoilTime;

	void Turn(float Rate);
	void LookUp(float Rate);

	void StartReload();
};
