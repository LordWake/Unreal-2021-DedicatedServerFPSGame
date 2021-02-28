//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginInteract, class ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndInteract, class ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginFocus, class ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndFocus, class ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, class ASurvivalCharacter*, Character);


 /*Every actor that uses this Interaction component can display an 
 ItemCard so the play can interact with this. Available to create if from blueprints.*/
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SURVIVALGAME_API UInteractionComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:

	UInteractionComponent();

	/* The time the player must hold the interact key to interact with this object */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionTime;

	/* The max distance the player can be away from this actor before can interact */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionDistance;

	/* The name that will show when the player looks at the interactable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractableNameText;

	/* The verb that describes how the interaction works, like "sit" for chair or "eat" for food objects */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractableActionText;

	/* Whether we allow multiple players to interact with the item, or just one at any given time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bAllowMultipleInteractors;

	//============================================================
	//=================== DELEGATES ==============================
	//============================================================

	/* [Local + Server] Called when the player presses the interact key while focusing on this interactable actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnBeginInteract OnBeginInteract;
	
	/* [Local + Server] Called when the player releases the interact key, stops looking at the interactable actor, or gets too far away */
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnEndInteract OnEndInteract;
	
	/* [Local + Server] Called when the player presses the interact key while focusing on this interactable actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnBeginFocus OnBeginFocus;
	
	/* [Local + Server] Called when the player releases the interact key, stops looking at the interactable actor, or gets too far away */
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnEndFocus OnEndFocus;
	
	/* [Local + Server] Called when the player has interacted with the item for the required amount of time. */
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnInteract	OnInteract;

protected:
	
	/*On the server, this will hold all the interactors. On the local player, this will just hold the local player (provided they are an interactor)*/
	UPROPERTY()
	TArray<class ASurvivalCharacter*> Interactors;

public:

	//Called on the client when the player's interaction check trace begins/ends hitting this item
	
	/*When you are looking at this  object*/
	void BeginFocus(class ASurvivalCharacter* Character);
	/*When you stop looking at this  object*/
	void EndFocus(class ASurvivalCharacter* Character);

	//Called on the client when the player begins/ends interaction withe the item
	
	/*Called when you look at the interactable and press the key*/
	void BeginInteract(class ASurvivalCharacter* Character);
	/*Called when you release the key*/
	void EndInteract(class ASurvivalCharacter* Character);

	/*When you finally do the interaction*/
	void Interact(class ASurvivalCharacter* Character);

	/*Returns 0 to 1 depends of how far through the interact we are. On server, this is the first interact percentage
	on client this is the local interact percentage. Handles the progress bar. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetInteractPercentage();

	/*Call this to change the name of the interactable. Will also refresh the interaction widget. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractableNameText(const FText& NewNameText);

	/*Call this to change the interaction text of the interactable. Will also refresh the interaction widget. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractableActionText(const FText& NewActionText);

	/*Refresh the interaction widget and it's custom widgets. An example of when we'd use this is when we take 3 items
	out of a stack of 10, and we need to update the widget so it shows the stack as having 7 items left.*/
	void RefreshWidget();

protected:

	/*Called when the game starts*/
	virtual void Deactivate() override;

	/*To check if a given character is allowed to interact*/
	bool CanInteract(class ASurvivalCharacter* Character) const;

};
