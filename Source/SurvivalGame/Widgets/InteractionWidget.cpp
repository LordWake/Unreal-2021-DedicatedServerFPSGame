//+---------------------------------------------------------+
//| Project   : Network Survival Game						|
//| UE Version: UE 4.25										|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "InteractionWidget.h"
#include "Components/InteractionComponent.h"

void UInteractionWidget::UpdateInteractionWidget(class UInteractionComponent* InteractionComponent)
{
	OwningInteractionComponent = InteractionComponent;
	OnUpdateInteractionWidget();
}
