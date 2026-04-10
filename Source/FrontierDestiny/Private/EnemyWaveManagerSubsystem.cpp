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

	UCoreManagerSubsystem* CoreManager = GetGameInstance()->GetWorld()->GetSubsystem<UCoreManagerSubsystem>();
	const int32 CapturedCount = CoreManager ? CoreManager->GetCapturedCoreCount() : 0;

	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: CapturedCoreCount = %d"), CapturedCount);

	for (const auto& Pair : Batch.SpawnMap)
	{
		const TArray<FHordeBatchDetails>& PerCore = Pair.Value.PerCoreCounts;
		if (PerCore.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyWaveManager: SpawnMap entry '%s' has empty PerCoreCounts — skipping."), *Pair.Key.ToString());
			continue;
		}

		const int32 Index = FMath::Clamp(CapturedCount - 1, 0, PerCore.Num() - 1);
		UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Broadcasting spawn order — Tag '%s', CoreIndex %d, Enemies: %d"),
			*Pair.Key.ToString(), Index, PerCore[Index].Enemies.Num());

		OnSpawnOrderIssued.Broadcast(Pair.Key, PerCore[Index]);
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
	TArray<FTimerHandle>* Handles = ActiveHordeTimers.Find(HordeID);
	if (!Handles)
	{
		return;
	}

	FTimerManager& TimerManager = GetGameInstance()->GetWorld()->GetTimerManager();
	for (FTimerHandle& Handle : *Handles)
	{
		TimerManager.ClearTimer(Handle);
	}

	ActiveHordeTimers.Remove(HordeID);
	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Cancelled horde '%s'."), *HordeID.ToString());
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

	HordeEnemiesRemaining.FindOrAdd(HordeID) += Count;
	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Registered %d enemies for horde '%s' (total now: %d)."),
		Count, *HordeID.ToString(), HordeEnemiesRemaining[HordeID]);
}

void UEnemyWaveManagerSubsystem::HandleHordeEnemyDeath(FName HordeID)
{
	int32* Remaining = HordeEnemiesRemaining.Find(HordeID);
	if (!Remaining)
	{
		return;
	}

	(*Remaining)--;

	if (*Remaining <= 0)
	{
		HordeEnemiesRemaining.Remove(HordeID);
		UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Horde '%s' cleared!"), *HordeID.ToString());

		OnHordeFinished.Broadcast(HordeID);

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
