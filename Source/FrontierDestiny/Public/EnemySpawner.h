// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "WaveData.h"
#include "GameplayTagContainer.h"
#include "CoreMinimal.h"
#include "MassSpawner.h"
#include "EnemyWaveManagerSubsystem.h"
#include "EnemySpawner.generated.h"

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API AEnemySpawner : public AMassSpawner
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadonly, EditAnywhere, Category = "Setup")
	FGameplayTag Tag;

	UPROPERTY(BlueprintReadonly, EditAnywhere, Category = "Setup")
	int32 CoreIndex = 0;

	/** If true, this spawner is a backup and will never be chosen as a redirect target's backup. */
	UPROPERTY(BlueprintReadonly, EditAnywhere, Category = "Setup")
	bool bIsBackup = false;

	/** Tag of the spawner to redirect to when the player is too close. Ignored if bIsBackup is true. */
	UPROPERTY(BlueprintReadonly, EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsBackup"))
	FGameplayTag BackupSpawnerTag;

	/** Player must be further than this distance for this spawner to fire. Default: 2500 units. */
	UPROPERTY(BlueprintReadonly, EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsBackup", ClampMin = "0.0"))
	float DisableDistance = 2500.f;

	AEnemySpawner();

	virtual void BeginPlay() override;

	void Spawn(const FHordeBatchDetails& Details);

private:
	UFUNCTION()
	void HandleSpawnOrder(FGameplayTag SpawnTag, const FHordeBatchDetails& Details);
};
