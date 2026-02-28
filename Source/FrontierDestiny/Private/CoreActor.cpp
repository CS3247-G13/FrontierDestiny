// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreActor.h"
#include "GameFramework/PlayerController.h"
#include "MainHUD.h"


ACoreActor::ACoreActor()
{
 	PrimaryActorTick.bCanEverTick = false;
    InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableCompoennt"));
}

void ACoreActor::BeginPlay()
{
    Super::BeginPlay();
    if (InteractableComponent)
    {
        InteractableComponent->OnInteracted.AddDynamic(
            this,
            &ACoreActor::HandleInteraction
        );
    }
}

void ACoreActor::ApplyDamage(float Damage)
{
    CoreData.CurrentHP = FMath::Clamp(
        CoreData.CurrentHP - Damage,
        0.f,
        CoreData.MaximumHP
    );

    OnCoreHPChanged.Broadcast(this);
}

void ACoreActor::ActivateCore()
{
    UE_LOG(LogTemp, Warning,
        TEXT("Broadcasting from instance %s | Address: %p"),
        *GetName(),
        this
    );
    OnCoreActivated.Broadcast(this);
}

void ACoreActor::HandleInteraction(AActor* Interactor)
{
    ActivateCore();

    if (InteractableComponent)
    {
        InteractableComponent->DestroyComponent();
        InteractableComponent = nullptr;
    }
}