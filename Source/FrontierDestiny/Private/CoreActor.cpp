// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreActor.h"
#include "GameFramework/PlayerController.h"
#include "QuestSubsystem.h"
#include "MainHUD.h"
#include "CoreManagerSubsystem.h"


ACoreActor::ACoreActor()
{
 	PrimaryActorTick.bCanEverTick = false;
}

void ACoreActor::BeginPlay()
{
    Super::BeginPlay();

    if (UCoreManagerSubsystem* CoreManager = GetWorld()->GetSubsystem<UCoreManagerSubsystem>())
    {
        CoreManager->RegisterCore(this);
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

    if (CoreData.CurrentHP <= 0.f)
    {
        OnCoreDestroyed.Broadcast(this);
    }
}

void ACoreActor::ActivateCore()
{
    bIsCoreActive = true;
    OnCoreActivated.Broadcast(this);
    UQuestSubsystem* QuestSystem = GetGameInstance()->GetSubsystem<UQuestSubsystem>();
    if (QuestSystem)
    {
        if (QuestSystem->CurrentQuest.QuestID == "Quest_Two")
        {
            QuestSystem->CompleteObjective();
        }
    }
}
