// Fill out your copyright notice in the Description page of Project Settings.

#include "EconomySubsystem.h"

void UEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UPlayerManagerSubsystem>();
	PlayerManager = GetGameInstance()->GetSubsystem<UPlayerManagerSubsystem>();
	if (!PlayerManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("EconomySubsystem: Could not find PlayerManagerSubsystem."));
	}

	FTimerManagerTimerParameters TimerParams;
	TimerParams.bLoop = true;
	TimerParams.bMaxOncePerFrame = true;
	TimerParams.FirstDelay = -1.f;

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UEconomySubsystem::AddPassiveIncome, 1.f, TimerParams);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UEconomySubsystem::OnLevelChanged
	);
}


void UEconomySubsystem::OnLevelChanged(UWorld* World)
{
	CurrentFunds.Nullsteel = 0;
	CurrentFunds.Cryxite = 0;
	CurrentFunds.Gravstone = 0;
	OnFundsChanged.Broadcast(CurrentFunds, FResourceAmount{ -CurrentFunds.Cryxite, -CurrentFunds.Nullsteel, -CurrentFunds.Gravstone });
	PlayerManager = GetGameInstance()->GetSubsystem<UPlayerManagerSubsystem>();
}

void UEconomySubsystem::Deinitialize()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	Super::Deinitialize();
}

bool UEconomySubsystem::HasSufficientFunds(const FResourceAmount& Cost) const
{
	return CurrentFunds.CanAfford(Cost);
}

bool UEconomySubsystem::TryDeductFunds(const FResourceAmount& Cost)
{
	if (!HasSufficientFunds(Cost))
	{
		return false;
	}
	CurrentFunds -= Cost;
	OnFundsChanged.Broadcast(CurrentFunds, FResourceAmount{ -Cost.Cryxite, -Cost.Nullsteel, -Cost.Gravstone });
	return true;
}

void UEconomySubsystem::AddFunds(const FResourceAmount& Amount)
{
	CurrentFunds += Amount;
	OnFundsChanged.Broadcast(CurrentFunds, Amount);
}

void UEconomySubsystem::AddPassiveIncome()
{
	if (PlayerManager)
	{
		AddFunds(PlayerManager->PlayerData.PassiveIncomePerSecond);
	}
}
