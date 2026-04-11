// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ResourceAmount.h"
#include "Engine/DataTable.h"
#include "WaveData.generated.h"

/**
 * One group of same-type enemies to spawn simultaneously at a location.
 * Modifiers is a proof-of-concept flat list (e.g. {"Fast", "Nimble"}).
 */
USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FEnemySpawnEntry
{
	GENERATED_BODY()

	/** Matches a row in the enemy DataTable (e.g. "Drone", "Broodling"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName EnemyID = NAME_None;

	/** How many of this enemy to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 Count = 0;

	/** Modifier tags applied to each spawned enemy. Use Enemy.Modifier.* tags defined in EnemySpawner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
	FGameplayTagContainer Modifiers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FResourceAmount PerEnemyReward;

	// Numeric modifier values — only relevant if the corresponding tag is present
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float FastSpeedMultiplier       = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float StrongDamageMultiplier    = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float VitalityAmount            = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float KineticResistance         = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float LaserResistance          = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float ElectricResistance       = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float AmorphicDamageCap        = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float FragmentedChunkSize      = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers") float DistortionSpeedMultiplier = 1.5f;
};

/**
 * The set of enemies to spawn at a location for a specific number of captured cores.
 * Index 0 = 1 core captured, Index 1 = 2 cores, Index 2 = 3 cores.
 */
USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FHordeBatchDetails
{
	GENERATED_BODY()

	/** All enemy groups spawned simultaneously when this core-count condition is met. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<FEnemySpawnEntry> Enemies;
};

/**
 * Wrapper so TMap can hold TArray<FHordeBatchDetails> as a Blueprint-visible value type.
 */
USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FHordeBatchDetailsByCore
{
	GENERATED_BODY()

	/** One entry per possible core-count. Index 0 = 1 core, Index 1 = 2 cores, Index 2 = 3 cores. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<FHordeBatchDetails> PerCoreCounts;
};

/**
 * DataTable row for a single timed batch within a horde.
 *
 * A horde (e.g. Horde 1) is composed of multiple batches (1.1, 1.2 …).
 * Each batch fires after Delay seconds from the previous batch in the same horde.
 *
 * SpawnMap:
 *   Key   = spawn location tag (e.g. Spawn.Grassy.A)
 *   Value = array of FHordeBatchDetails, one entry per possible core-count
 *           [0] = 1 core captured, [1] = 2 cores, [2] = 3 cores
 *
 * Example from design doc:
 *   GrassyspawnA: [2/6/10]Drones, [3]Broodlings{Fast, Nimble}
 *   →  SpawnMap[Spawn.Grassy.A].PerCoreCounts[0].Enemies = [{Drone,2,[]}, {Broodling,3,[Fast,Nimble]}]
 *      SpawnMap[Spawn.Grassy.A].PerCoreCounts[1].Enemies = [{Drone,6,[]}, {Broodling,3,[Fast,Nimble]}]
 *      SpawnMap[Spawn.Grassy.A].PerCoreCounts[2].Enemies = [{Drone,10,[]},{Broodling,3,[Fast,Nimble]}]
 */
USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FWaveBatchRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Which horde this batch belongs to (e.g. "0", "1", "25"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName HordeID = NAME_None;

	/** Unique identifier for this batch within its horde (e.g. "0.1", "1.2"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName BatchID = NAME_None;

	/**
	 * Seconds to wait after the previous batch in this horde before spawning.
	 * For the first batch in a horde, this is relative to the horde start time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float Delay = 0.f;

	/**
	 * Spawn location tag → per-core-count enemy groups.
	 * The tag should correspond to a named spawn point actor in the level
	 * (e.g. Spawn.Grassy.A, Spawn.Volcanic.B, Spawn.Grav.X).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TMap<FGameplayTag, FHordeBatchDetailsByCore> SpawnMap;
};
