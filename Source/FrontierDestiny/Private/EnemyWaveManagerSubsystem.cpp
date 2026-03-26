// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyWaveManagerSubsystem.h"
#include "GlobalTowerSettings.h"
#include "CoreManagerSubsystem.h"

const TArray<FWaveBatchRow> UEnemyWaveManagerSubsystem::EmptyBatchArray;

void UEnemyWaveManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadWaveDataFromDataTable();
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

	UCoreManagerSubsystem* CoreManager = GetWorld()->GetSubsystem<UCoreManagerSubsystem>();
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

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
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

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	for (FTimerHandle& Handle : *Handles)
	{
		TimerManager.ClearTimer(Handle);
	}

	ActiveHordeTimers.Remove(HordeID);
	UE_LOG(LogTemp, Log, TEXT("EnemyWaveManager: Cancelled horde '%s'."), *HordeID.ToString());
}
