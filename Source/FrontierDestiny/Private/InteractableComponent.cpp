// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableComponent.h"

void UInteractableComponent::BeginPlay()
{
    Super::BeginPlay();
}

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableComponent::Interact(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("%s was interacted with!"),*GetOwner()->GetName());
	OnInteracted.Broadcast(Interactor);
}

