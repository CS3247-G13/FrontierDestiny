// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractorComponent.h"
#include "InteractableComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CheckForInteractable();
}

void UInteractorComponent::CheckForInteractable()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (!Camera) return;

	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	if (!bHit)
	{
		OnInteractableLost.Broadcast();
		CurrentInteractable = nullptr;
		NewInteractable = nullptr;
	}

	if (bHit && Hit.GetActor())
	{
		NewInteractable = Hit.GetActor()->FindComponentByClass<UInteractableComponent>();
		if (IsValid(NewInteractable) && !NewInteractable->GetIsInteractable())
		{
			NewInteractable = nullptr;
		}
	}

	if (CurrentInteractable != NewInteractable)
	{
		if (CurrentInteractable)
		{
			OnInteractableLost.Broadcast();
		}

		CurrentInteractable = NewInteractable;
		if (CurrentInteractable)
		{
			OnInteractableFocused.Broadcast(CurrentInteractable);
		}
	}
}

void UInteractorComponent::Interact()
{
	if (CurrentInteractable)
	{
		CurrentInteractable->Interact(GetOwner());
		OnInteractableLost.Broadcast();
	}
}