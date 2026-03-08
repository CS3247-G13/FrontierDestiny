// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableComponent.h"

void UInteractableComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UInteractableComponent::ActivateInteractable()
{
	bIsInteractable = true;
}

void UInteractableComponent::DeactivateInteractable()
{
	bIsInteractable = false;
}

bool UInteractableComponent::GetIsInteractable()
{
	return bIsInteractable;
}

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableComponent::Interact(AActor* Interactor)
{
	if (bIsInteractable)
	{
		OnInteracted.Broadcast(Interactor);
		if (bIsOneTimeUse)
		{
			DestroyComponent();
		}
	}
	else
	{
		return;
	}
}

