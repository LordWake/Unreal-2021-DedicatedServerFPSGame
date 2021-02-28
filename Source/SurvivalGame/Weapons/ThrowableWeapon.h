//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThrowableWeapon.generated.h"

/*A weapon we can throw.*/
UCLASS()
class SURVIVALGAME_API AThrowableWeapon : public AActor
{
	GENERATED_BODY()
	
public:	

	AThrowableWeapon();

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UStaticMeshComponent* ThrowableMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UProjectileMovementComponent* ThrowableMovement;

};
