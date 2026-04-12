// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreActor.h"
#include "GameFramework/PlayerController.h"
#include "QuestSubsystem.h"
#include "MainHUD.h"
#include "CoreManagerSubsystem.h"
#include "Components/PrimitiveComponent.h"


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

void ACoreActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(RegenTimerHandle);
    Super::EndPlay(EndPlayReason);
}

void ACoreActor::TickRegen()
{
    if (CoreData.CurrentHP >= CoreData.MaximumHP)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (LastDamageTime >= 0.f && (Now - LastDamageTime) < DamageGracePeriod)
    {
        return;
    }

    CoreData.CurrentHP = FMath::Min(CoreData.CurrentHP + HPRegenRate, CoreData.MaximumHP);
    OnCoreHPChanged.Broadcast(this);
}

void ACoreActor::ApplyDamage(float Damage)
{
    if (CoreData.CurrentHP <= 0.f)
    {
        return;
	}

    LastDamageTime = GetWorld()->GetTimeSeconds();

    CoreData.CurrentHP = FMath::Clamp(
        CoreData.CurrentHP - Damage,
        0.f,
        CoreData.MaximumHP
    );

    OnCoreHPChanged.Broadcast(this);

    if (CoreData.CurrentHP <= 0.f)
    {
        OnCoreDestroyed.Broadcast(this);
        GetWorldTimerManager().ClearTimer(RegenTimerHandle);
    }
}

FVector ACoreActor::GetTeleportPoint() const
{
	if (IsValid(TeleportPoint))
	{
		return TeleportPoint->GetActorLocation();
	}
	return GetActorLocation();
}

void ACoreActor::SetShownThroughWalls(bool bEnable)
{
	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (bEnable)
		{
			Prim->SetCustomDepthStencilValue(37);
			Prim->SetRenderCustomDepth(true);
		}
		else
		{
			Prim->SetRenderCustomDepth(false);
		}
	}
}

void ACoreActor::SetHovered(bool bHovered)
{
	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		Prim->SetCustomDepthStencilValue(bHovered ? 38 : 37);
		Prim->SetRenderCustomDepth(true);
	}
}

void ACoreActor::ActivateCore()
{
    bIsCoreActive = true;
    GetWorldTimerManager().SetTimer(RegenTimerHandle, this, &ACoreActor::TickRegen, 1.f, true);
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
