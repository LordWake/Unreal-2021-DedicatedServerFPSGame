//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Items/EquippableItem.h"
#include "GearItem.generated.h"

/*Everything that we can equip  is a GearItem. Like clothes.*/
UCLASS(Blueprintable)
class SURVIVALGAME_API UGearItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UGearItem();

	virtual bool Equip(class ASurvivalCharacter* Character) override;
	virtual bool UnEquip(class ASurvivalCharacter* Character) override;

	/*The skeletal mesh for this gear (a T-Shirt, a Helmet, Pants, etc.)*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class USkeletalMesh* Mesh;

	/*Optional material instance to apply to the gear.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class UMaterialInstance* MaterialInstance;

	/*The amount of defense this item provides. 0.2 = 20% less damage taken. Works like a bulletproof.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float DamageDefenseMultiplier;
	
};
