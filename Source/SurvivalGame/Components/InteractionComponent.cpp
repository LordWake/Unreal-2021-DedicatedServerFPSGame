//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "InteractionComponent.h"

#include "Player/SurvivalCharacter.h"
#include "Widgets/InteractionWidget.h"

UInteractionComponent::UInteractionComponent()
{
	SetComponentTickEnabled(false);

	InteractionTime = 0.f;
	InteractionDistance = 200.f;

	InteractableNameText = FText::FromString("Interactable Object");
	InteractableActionText = FText::FromString("Interact");

	bAllowMultipleInteractors = true;

	Space = EWidgetSpace::Screen; 
	DrawSize = FIntPoint(600, 100); //How big the UI is gonna be
	bDrawAtDesiredSize = true; 

	SetActive(true);
	SetHiddenInGame(true);
}

void UInteractionComponent::BeginFocus(class ASurvivalCharacter* Character)
{
	if (!IsActive() || !GetOwner() || !Character)
	{
		return;
	}

	OnBeginFocus.Broadcast(Character);


	//If this is not the server.
	if (GetNetMode() != NM_DedicatedServer)
	{
		SetHiddenInGame(false);
		
		//Take any visual component. This is the equivalent of GetComponentsByClass
		TInlineComponentArray<UPrimitiveComponent*> AllVisualComponents(GetOwner());
		for (auto& VisualComp : AllVisualComponents)
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
			{
				Prim->SetRenderCustomDepth(true); //And this is gonna enable the outline effect.
			}
		}
		
		delete &AllVisualComponents;
	}

	RefreshWidget();
}

void UInteractionComponent::EndFocus(class ASurvivalCharacter* Character)
{
	OnEndFocus.Broadcast(Character);

	//If this is not the server.
	if (GetNetMode() != NM_DedicatedServer)
	{
		SetHiddenInGame(true); //Hide UI

		//Take any visual component. This is the equivalent of GetComponentsByClass
		TInlineComponentArray<UPrimitiveComponent*> AllVisualComponents(GetOwner());
		for (auto& VisualComp : AllVisualComponents)
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
			{
				Prim->SetRenderCustomDepth(false); //And this is gonna disable the outline effect.
			}
		}

		delete &AllVisualComponents;
	}
}

void UInteractionComponent::BeginInteract(class ASurvivalCharacter* Character)
{
	if(CanInteract(Character))
	{
		Interactors.AddUnique(Character);
		OnBeginInteract.Broadcast(Character);
	}
}

void UInteractionComponent::EndInteract(class ASurvivalCharacter* Character)
{
	Interactors.RemoveSingle(Character);
	OnEndInteract.Broadcast(Character);
}

void UInteractionComponent::Interact(class ASurvivalCharacter* Character)
{
	if (CanInteract(Character))
	{
		OnInteract.Broadcast(Character);
	}
}


bool UInteractionComponent::CanInteract(class ASurvivalCharacter* Character) const
{
	//if we allow multiple and interactors and have more than one
	const bool bPlayerAlreadyInteracting = !bAllowMultipleInteractors && Interactors.Num() >= 1;

	return !bPlayerAlreadyInteracting && IsActive() && GetOwner() != nullptr && Character != nullptr;
}

float UInteractionComponent::GetInteractPercentage()
{
	//Get the first interactor
	if (Interactors.IsValidIndex(0))
	{
		if (ASurvivalCharacter* Interactor = Interactors[0])
		{
			//Check if we are interacting
			if (Interactor && Interactor->IsInteracting())
			{
				return 1.f - FMath::Abs(Interactor->GetRemainingInteractTime() / InteractionTime);
			}
		}
	}
	
	return 0.f;
}

void UInteractionComponent::SetInteractableNameText(const FText& NewNameText)
{
	InteractableNameText = NewNameText;
	RefreshWidget();
}

void UInteractionComponent::SetInteractableActionText(const FText& NewActionText)
{
	InteractableActionText = NewActionText;
	RefreshWidget();
}

void UInteractionComponent::RefreshWidget()
{
	//If it isn't hidden or it is not the server (the server doesn't need to update UI)
	if (!bHiddenInGame && GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		//Make sure the widget is initialized, and that we are displaying the right values(these may have changed)
		if (UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUserWidgetObject()))
		{
			InteractionWidget->UpdateInteractionWidget(this);
		}
	}
}

void UInteractionComponent::Deactivate()
{
	Super::Deactivate();

	//Disable focus and interact with all the player that are inside interactors array
	for (int32 i = Interactors.Num() - 1; i >= 0; --i)
	{
		if (ASurvivalCharacter* Interactor = Interactors[i])
		{
			EndFocus(Interactor);
			EndInteract(Interactor);
		}
	}

	Interactors.Empty();
}