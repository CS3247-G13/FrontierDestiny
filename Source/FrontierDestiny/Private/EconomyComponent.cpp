// Fill out your copyright notice in the Description page of Project Settings.


#include "EconomyComponent.h"

UEconomyComponent::UEconomyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEconomyComponent::BeginPlay()
{
	Super::BeginPlay();

	FTimerManagerTimerParameters TimerParams;
	TimerParams.bLoop = true;
	TimerParams.bMaxOncePerFrame = true;
	TimerParams.FirstDelay = -1.f;

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UEconomyComponent::AddPassiveIncome, 1.f, TimerParams);
}

bool UEconomyComponent::HasSufficientFunds(int32 Amount) const
{
	return CurrentFunds >= Amount;
}

bool UEconomyComponent::TryDeductFunds(int32 Amount)
{
	if (!HasSufficientFunds(Amount))
	{
		return false;
	}
	CurrentFunds -= Amount;
	OnFundsChanged.Broadcast(CurrentFunds, -Amount);
	return true;
}

void UEconomyComponent::AddFunds(int32 Amount)
{
	CurrentFunds += Amount;
	OnFundsChanged.Broadcast(CurrentFunds, Amount);
}

void UEconomyComponent::AddPassiveIncome()
{
	AddFunds(PassiveIncomePerSecond);
}