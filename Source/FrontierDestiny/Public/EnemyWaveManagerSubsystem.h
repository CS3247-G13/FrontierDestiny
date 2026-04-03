// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WaveData.h"
#include "HordeData.h"
#include "EnemyWaveManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpawnOrderIssued, FGameplayTag, SpawnTag, const FHordeBatchDetails&, Details);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHordeBegin, FName, HordeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHordeBatchBegin, FName, HordeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHordeFinished, FName, HordeID);

/**
 * Loads and owns all wave batch data for the current level.
 * Data is grouped by HordeID for efficient lookup during wave execution.
 */
UCLASS()
class FRONTIERDESTINY_API UEnemyWaveManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * All batches for a given horde, in the order they were defined in the DataTable.
	 * Key = HordeID (e.g. "0", "1"). Value = ordered list of batches within that horde.
	 */
	TMap<FName, TArray<FWaveBatchRow>> HordeBatchMap;

	TMap<FName, FHordeDataRow> HordeDataMap;

	/** Returns all batches for the given horde, or an empty array if not found. */
	const TArray<FWaveBatchRow>& GetBatchesForHorde(const FName& HordeID) const;

	/** Broadcasts the given batch to all subscribed spawners. */
	UFUNCTION(BlueprintCallable)
	void TriggerBatch(const FWaveBatchRow& Batch);

	/**
	 * Schedules all batches for the given horde using individual timers.
	 * Delays are accumulated so each batch fires at the correct absolute time.
	 * If the horde is already running, it is cancelled and restarted.
	 */
	UFUNCTION(BlueprintCallable)
	void StartHorde(FName HordeID);

	/** Cancels all pending batch timers for the given horde. */
	UFUNCTION(BlueprintCallable)
	void CancelHorde(FName HordeID);

	UPROPERTY(BlueprintAssignable)
	FOnSpawnOrderIssued OnSpawnOrderIssued;

	/** Fired just before a horde's first spawn order is issued, so BGM can change to horde music. */
	UPROPERTY(BlueprintAssignable)
	FOnHordeBegin OnHordeBegin;

	/** Fired just before a batch's spawn orders are issued, so spawners can cache the current HordeID. */
	UPROPERTY(BlueprintAssignable)
	FOnHordeBatchBegin OnHordeBatchBegin;

	/** Fired when all enemies belonging to a horde have been killed. */
	UPROPERTY(BlueprintAssignable)
	FOnHordeFinished OnHordeFinished;

	/** Called by EnemySpawner after DoSpawning() to register how many enemies were spawned for a horde. */
	void RegisterSpawnedEnemies(FName HordeID, int32 Count);

private:
	void LoadWaveDataFromDataTable();
	void LoadHordeDataFromDataTable();

	// Returned by GetBatchesForHorde when the horde ID doesn't exist.
	static const TArray<FWaveBatchRow> EmptyBatchArray;

	// Active horde timers. Key = HordeID, Value = one handle per batch.
	TMap<FName, TArray<FTimerHandle>> ActiveHordeTimers;

	// Remaining enemy count per active horde. Decremented on each horde enemy death.
	TMap<FName, int32> HordeEnemiesRemaining;

	void HandleHordeEnemyDeath(FName HordeID);
};
