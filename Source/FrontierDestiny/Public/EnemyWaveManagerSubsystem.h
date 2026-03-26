// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WaveData.h"
#include "EnemyWaveManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpawnOrderIssued, FGameplayTag, SpawnTag, const FHordeBatchDetails&, Details);

/**
 * Loads and owns all wave batch data for the current level.
 * Data is grouped by HordeID for efficient lookup during wave execution.
 */
UCLASS()
class FRONTIERDESTINY_API UEnemyWaveManagerSubsystem : public UWorldSubsystem
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

private:
	void LoadWaveDataFromDataTable();

	// Returned by GetBatchesForHorde when the horde ID doesn't exist.
	static const TArray<FWaveBatchRow> EmptyBatchArray;

	// Active horde timers. Key = HordeID, Value = one handle per batch.
	TMap<FName, TArray<FTimerHandle>> ActiveHordeTimers;
};
