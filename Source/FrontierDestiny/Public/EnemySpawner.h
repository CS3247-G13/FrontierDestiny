// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "WaveData.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "CoreMinimal.h"
#include "MassSpawner.h"
#include "StatusEffectFragments.h"
#include "EnemyWaveManagerSubsystem.h"
#include "EnemySpawner.generated.h"

// Native gameplay tags for enemy modifiers — use these instead of FName strings
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Fast)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Strong)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Vitality)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Armoured)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Reflective)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Insulated)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Stealthy)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Nimble)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Pyroclastic)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Amorphic)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Distorted)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modifier_Fragmented)

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

	UFUNCTION()
	void HandleHordeBatchBegin(FName HordeID);

	UFUNCTION()
	void HandleSpawningFinished();

	FName CurrentHordeID;
	int32 SpawnStartIndex = 0;

	/** Modifier fragments built in Spawn(), consumed in HandleSpawningFinished(). One entry per EntityTypes entry. */
	TArray<FModifierFragment> PendingModifierFragments;
	/** Whether each pending entry actually has any modifiers set. */
	TArray<bool> PendingHasModifiers;
	/** Base stats from EnemyData.Attributes, set before multipliers are applied. */
	TArray<float> PendingBaseHP;
	TArray<float> PendingBaseSpeed;
	TArray<float> PendingBaseDamage;
	/** Stat multipliers to bake into FStatsFragment at spawn. */
	TArray<float> PendingSpeedMultipliers;
	TArray<float> PendingDamageMultipliers;
	TArray<float> PendingVitalityAmounts;
};
