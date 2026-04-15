// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyWaveManagerSubsystem.h"
#include "GlobalTowerSettings.h"
#include "CoreManagerSubsystem.h"
#include "EnemyManagerSubsystem.h"
#include "EconomySubsystem.h"

const TArray<FWaveBatchRow> UEnemyWaveManagerSubsystem::EmptyBatchArray;

void UEnemyWaveManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadWaveDataFromDataTable();
	LoadHordeDataFromDataTable();

	if (UEnemyManagerSubsystem* EnemyManager = GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>())
	{
		EnemyManager->OnHordeEnemyDeath.AddUObject(this, &UEnemyWaveManagerSubsystem::HandleHordeEnemyDeath);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UEnemyWaveManagerSubsystem::OnLevelChanged
	);
}

void UEnemyWaveManagerSubsystem::OnLevelChanged(UWorld* World)
{
	HordeBatchMap.Empty();
	HordeDataMap.Empty();
	ActiveHordeTimers.Empty();
	UpcomingHordeTimers.Empty();
	HordeEnemiesRemaining.Empty();


	LoadWaveDataFromDataTable();
	LoadHordeDataFromDataTable();
}

void UEnemyWaveManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UEnemyWaveManagerSubsystem::LoadWaveDataFromDataTable()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UDataTable* Table = Settings->WaveDataTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyWaveManager: No WaveDataTable set in Project Settings > Global Tower Settings."));
		return;
	}

	const TMap<FName, uint8*>& RowMap = Table->GetRowMap();

	for (const auto& Pair : RowMap)
	{
		FWaveBatchRow* Row = reinterpret_cast<FWaveBatchRow*>(Pair.Value);
		if (!Row)
		{
			continue;
		}

		if (Row->HordeID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyWaveManager: Row '%s' has no HordeID set — skipping."), *Pair.Key.ToString());
			continue;
		}

		HordeBatchMap.FindOrAdd(Row->HordeID).Add(*Row);
	}

	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Loaded %d hordes from DataTable."), HordeBatchMap.Num());
}

const TArray<FWaveBatchRow>& UEnemyWaveManagerSubsystem::GetBatchesForHorde(const FName& HordeID) const
{
	const TArray<FWaveBatchRow>* Found = HordeBatchMap.Find(HordeID);
	return Found ? *Found : EmptyBatchArray;
}

void UEnemyWaveManagerSubsystem::TriggerBatch(const FWaveBatchRow& Batch)
{
	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: TriggerBatch — BatchID '%s' (HordeID '%s'), SpawnMap entries: %d"),
		*Batch.BatchID.ToString(), *Batch.HordeID.ToString(), Batch.SpawnMap.Num());

	if (Batch.BatchID == FName("0")) {
		OnHordeBegin.Broadcast(Batch.HordeID);
	}
	
	OnHordeBatchBegin.Broadcast(Batch.HordeID);

	for (const auto& Pair : Batch.SpawnMap)
	{
		UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Broadcasting spawn order — Tag '%s', Enemies: %d"),
			*Pair.Key.ToString(), Pair.Value.Enemies.Num());

		OnSpawnOrderIssued.Broadcast(Pair.Key, Pair.Value);
	}
}

void UEnemyWaveManagerSubsystem::StartHorde(FName HordeID)
{
	// Cancel any existing run of this horde before starting fresh.
	CancelHorde(HordeID);

	const TArray<FWaveBatchRow>& Batches = GetBatchesForHorde(HordeID);
	if (Batches.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyWaveManager: StartHorde called for unknown HordeID '%s'."), *HordeID.ToString());
		return;
	}

	TArray<FTimerHandle>& Handles = ActiveHordeTimers.Add(HordeID);
	Handles.Reserve(Batches.Num());

	FTimerManager& TimerManager = GetGameInstance()->GetWorld()->GetTimerManager();
	float AbsoluteTime = 0.f;

	for (const FWaveBatchRow& Batch : Batches)
	{
		AbsoluteTime += Batch.Delay;

		if (AbsoluteTime <= 0.f)
		{
			TriggerBatch(Batch);
			continue;
		}

		FTimerHandle Handle;
		FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this, Batch]()
		{
			TriggerBatch(Batch);
		});

		TimerManager.SetTimer(Handle, Delegate, AbsoluteTime, false);
		Handles.Add(Handle);
	}


	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Started horde '%s' — %d batches scheduled."), *HordeID.ToString(), Batches.Num());
}

void UEnemyWaveManagerSubsystem::CancelHorde(FName HordeID)
{
	FTimerManager& TimerManager = GetGameInstance()->GetWorld()->GetTimerManager();

	// Cancel batch timers for the horde itself
	if (TArray<FTimerHandle>* Handles = ActiveHordeTimers.Find(HordeID))
	{
		for (FTimerHandle& Handle : *Handles)
		{
			TimerManager.ClearTimer(Handle);
		}
		ActiveHordeTimers.Remove(HordeID);
		UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Cancelled horde '%s'."), *HordeID.ToString());
	}

	// If this horde was queued as a next-horde countdown, cancel that too and notify listeners
	if (FTimerHandle* UpcomingHandle = UpcomingHordeTimers.Find(HordeID))
	{
		TimerManager.ClearTimer(*UpcomingHandle);
		UpcomingHordeTimers.Remove(HordeID);
		OnNextHordeCancelled.Broadcast(HordeID);
		UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Cancelled upcoming horde countdown for '%s'."), *HordeID.ToString());
	}
}

void UEnemyWaveManagerSubsystem::LoadHordeDataFromDataTable()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UDataTable* Table = Settings->HordeDataTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyWaveManager: No HordeDataTable set in Project Settings > Global Tower Settings."));
		return;
	}

	const TMap<FName, uint8*>& RowMap = Table->GetRowMap();
	for (const auto& Pair : RowMap)
	{
		FHordeDataRow* Row = reinterpret_cast<FHordeDataRow*>(Pair.Value);
		if (!Row || Row->HordeID.IsNone())
		{
			continue;
		}
		HordeDataMap.Add(Row->HordeID, *Row);
	}

	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Loaded %d horde reward entries."), HordeDataMap.Num());
}


void UEnemyWaveManagerSubsystem::RegisterSpawnedEnemies(FName HordeID, int32 Count)
{
	if (HordeID.IsNone() || Count <= 0)
	{
		return;
	}

	const int32 NewTotal = HordeEnemiesRemaining.FindOrAdd(HordeID) += Count;
	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Registered %d enemies for horde '%s' (total now: %d)."),
		Count, *HordeID.ToString(), NewTotal);
	OnHordeEnemyCountChanged.Broadcast(HordeID, NewTotal);
}

void UEnemyWaveManagerSubsystem::HandleHordeEnemyDeath(FName HordeID)
{
	int32* Remaining = HordeEnemiesRemaining.Find(HordeID);
	if (!Remaining)
	{
		return;
	}

	(*Remaining)--;
	OnHordeEnemyCountChanged.Broadcast(HordeID, *Remaining);

	if (*Remaining <= 0)
	{
		HordeEnemiesRemaining.Remove(HordeID);
		UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Horde '%s' cleared!"), *HordeID.ToString());

		OnHordeFinished.Broadcast(HordeID);
		ReadyNextHorde(HordeID);

		const FHordeDataRow* HordeData = HordeDataMap.Find(HordeID);
		if (!HordeData)
		{
			UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Horde '%s' not found!"), *HordeID.ToString());
			return;
		}
		UCoreManagerSubsystem* CoreManager = GetGameInstance()->GetWorld()->GetSubsystem<UCoreManagerSubsystem>();
		if (!CoreManager)
		{
			UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Core Manager not found!"));
			return;
		}
		UEconomySubsystem* Economy = GetGameInstance()->GetSubsystem<UEconomySubsystem>();
		if (!Economy)
		{
			UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Economy Manager not found!"));
			return;
		}
		FResourceAmount TotalRewards;
		for (int i = 0; i < TOTALCORECOUNT; i++)
		{
			if (CoreManager->IsCoreCaptured(i))
			{
				UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Core %d is unlocked!"), i);
				if (i < HordeData->PerCoreReward.Num())
				{
					UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Core %d has an entry"), i);
					TotalRewards += HordeData->PerCoreReward[i];
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Core %d not unlocked!"), i);
			}
		}
		Economy->AddFunds(TotalRewards);
	}
}

void UEnemyWaveManagerSubsystem::ReadyNextHorde(FName EndedHordeID)
{
	const FHordeDataRow* HordeData = HordeDataMap.Find(EndedHordeID);
	if (!HordeData || HordeData->NextHordeID == NAME_None)
	{
		return;
	}

	const FName NextHordeID = HordeData->NextHordeID;
	if (FTimerHandle* ExistingHandle = UpcomingHordeTimers.Find(NextHordeID))
	{
		return;
	}

	FTimerHandle Handle;

	FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this, NextHordeID]()
	{
		UpcomingHordeTimers.Remove(NextHordeID);
		StartHorde(NextHordeID);
	});

	const float Delay = HordeData->TimeInSecondsToNextHorde;

	FTimerManager& TimerManager = GetGameInstance()->GetWorld()->GetTimerManager();
	TimerManager.SetTimer(Handle, Delegate, Delay, false);
	UpcomingHordeTimers.Add(NextHordeID, Handle);

	OnNextHordeScheduled.Broadcast(NextHordeID, Delay);
}