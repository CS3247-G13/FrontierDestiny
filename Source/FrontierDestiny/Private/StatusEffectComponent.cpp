// Fill out your copyright notice in the Description page of Project Settings.

#include "StatusEffectComponent.h"
#include "EnemyActor.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UStatusEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearAllStatusEffects();
    Super::EndPlay(EndPlayReason);
}

void UStatusEffectComponent::ClearAllStatusEffects()
{
    for (auto& Elem : ActiveDOTTimers)
    {
        FStatusEffectTimer& EffectTimer = Elem.Value;
        GetWorld()->GetTimerManager().ClearTimer(EffectTimer.TimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(EffectTimer.ExpiryHandle);
    }
    ActiveDOTTimers.Empty();
}

void UStatusEffectComponent::ApplyDOTByDamagePerTick(FName Tag, float DamagePerTick, float Duration, float Interval, AController* Instigator, AActor* DamageCauser)
{
    // 1. Setup the Damage Loop
	FStatusEffectTimer& EffectTimer = ActiveDOTTimers.FindOrAdd(Tag);

    FTimerHandle& DamageHandle = EffectTimer.TimerHandle;
    FTimerDelegate DamageDel;
    DamageDel.BindUObject(this, &UStatusEffectComponent::DamageEntity, Tag);

    GetWorld()->GetTimerManager().SetTimer(DamageHandle, DamageDel, Interval, true);

    // 2. Setup the Expiry Handler
    FTimerHandle& ExpiryHandle = EffectTimer.ExpiryHandle; // You can also store this in a Map if you need to cancel it early
    FTimerDelegate ExpiryDel;
    ExpiryDel.BindUObject(this, &UStatusEffectComponent::HandleEffectExpired, Tag);

	EffectTimer.Instigator = Instigator; // Store the instigator for potential use in the expiry handler or elsewhere
	EffectTimer.DamageCauser = DamageCauser; // Store the damage causer for potential use in the expiry handler or elsewhere
	EffectTimer.Damage = DamagePerTick; // Store the damage per tick for use in the DamageEntity function

    // This fires once after 'Duration'
    GetWorld()->GetTimerManager().SetTimer(ExpiryHandle, ExpiryDel, Duration, false);
}

void UStatusEffectComponent::ApplyDOTByTotalDamage(FName Tag, float TotalDamage, float Duration, float Interval, AController* Instigator, AActor* DamageCauser)
{
	ApplyDOTByDamagePerTick(Tag, TotalDamage / (Duration / Interval), Duration, Interval, Instigator, DamageCauser);
}

void UStatusEffectComponent::ApplyDOTByDamagePerSecond(FName Tag, float DamagePerSecond, float Duration, float Interval, AController* Instigator, AActor* DamageCauser)
{
	ApplyDOTByDamagePerTick(Tag, DamagePerSecond * Interval, Duration, Interval, Instigator, DamageCauser);
}

void UStatusEffectComponent::DamageEntity(FName Tag)
{
    if (!ActiveDOTTimers.Contains(Tag))
    {
        return; // No active DOT with this tag, should not happen but just in case
    }
    FStatusEffectTimer& EffectTimer = ActiveDOTTimers[Tag];

	UGameplayStatics::ApplyDamage(GetOwner(), EffectTimer.Damage, EffectTimer.Instigator, EffectTimer.DamageCauser, UDamageType::StaticClass());
}

void UStatusEffectComponent::HandleEffectExpired(FName Tag)
{
    if (ActiveDOTTimers.Contains(Tag))
    {
        FStatusEffectTimer& EffectTimer = ActiveDOTTimers[Tag];
        // Stop the looping damage
        GetWorld()->GetTimerManager().ClearTimer(EffectTimer.TimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(EffectTimer.ExpiryHandle);
        ActiveDOTTimers.Remove(Tag);
    }
}