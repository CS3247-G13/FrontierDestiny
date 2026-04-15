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
};

/**
 * The set of enemies to spawn at a location for a batch.
 */
USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FHordeBatchDetails
{
	GENERATED_BODY()

	/** All enemy groups spawned simultaneously at this spawn point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<FEnemySpawnEntry> Enemies;
};

/**
 * DataTable row for a single timed batch within a horde.
 *
 * A horde (e.g. Horde 1) is composed of multiple batches (1.1, 1.2 …).
 * Each batch fires after Delay seconds from the previous batch in the same horde.
 *
 * SpawnMap:
 *   Key   = spawn location tag (e.g. Spawn.Grassy.A)
 *   Value = the enemy groups to spawn simultaneously at that location
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
	 * Spawn location tag → enemy groups to spawn at that location.
	 * The tag should correspond to a named spawn point actor in the level
	 * (e.g. Spawn.Grassy.A, Spawn.Volcanic.B, Spawn.Grav.X).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TMap<FGameplayTag, FHordeBatchDetails> SpawnMap;
};
