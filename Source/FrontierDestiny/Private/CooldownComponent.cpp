// Fill out your copyright notice in the Description page of Project Settings.


#include "CooldownComponent.h"

// Sets default values for this component's properties
UCooldownComponent::UCooldownComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called every frame
void UCooldownComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsActive)
	{
		ElapsedTime = FMath::Min(CooldownDuration, ElapsedTime + DeltaTime * CooldownMultiplier);

		OnCooldownTick.Broadcast(GetProgressPercent());

		if (GetRemainingTime() < KINDA_SMALL_NUMBER)
		{
			// Cooldown finished
			OnCooldownFinished.Broadcast();

			switch (Mode) {
			case ECooldownMode::DeactivateOnFinish:
				PauseCooldown();
				break;
			case ECooldownMode::LoopOnFinish:
				ResetCooldown();
				StartCooldown();
				break;
			case ECooldownMode::TickOnFinish:
				break;
			}
		}
	}
}

void UCooldownComponent::StartCooldown()
{
	bIsActive = true;
}

void UCooldownComponent::PauseCooldown()
{
	bIsActive = false;
}

void UCooldownComponent::ResetCooldown()
{
	PauseCooldown();
	ElapsedTime = 0.f;
}

