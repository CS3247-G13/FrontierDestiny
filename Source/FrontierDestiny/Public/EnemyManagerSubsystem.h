// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnemyData.h"
#include "CoreMinimal.h"
#include "MassEnemyTarget.h"
#include "NiagaraComponent.h"
#include "MassEntityHandle.h"
#include "EnemyDamageMassProcessor.h"
#include "ResourceAmount.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "EnemyManagerSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyDeathSignature, FMassEntityHandle);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHordeEnemyDeathSignature, FName);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnemyDamageTakenSignature, FVector, int32);

struct FEnemySpawnRequest
{
	FMassEntityHandle SourceEntity;
	FName EnemyID;
	FTransform BaseTransform;
	int32 Count = 0;
	float Radius = 0.f;
};

UCLASS()
class FRONTIERDESTINY_API UEnemyManagerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Healthbars
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	// Per-slot arrays pushed to Niagara every frame
	TArray<float> EnemyHealths;
	TArray<float> EnemyMaxHealths;
	TArray<FVector> EnemyPositions;
	TArray<float> EnemyVisibilities;
	TArray<float> EnemyVitalities;
	TArray<float> BurnDurations;
	TArray<float> SlowDurations;
	TArray<float> StunDurations;
	TArray<float> ModifierFlags;           // booleans packed as int cast to float
	TArray<float> FragmentedChunkSizes;
	TArray<float> MitigatedDamageAmounts; // fading mitigated damage bar (amorphic + fragmented)

	// Internal fade state — not pushed to Niagara
	TArray<float> MitigatedDamageFadeTimers;
	TArray<float> EnemyHeights;

	void InitializeHealthbars();

	FMassEntityHandle GetEnemyEntityHandle(UInstancedStaticMeshComponent* Component, int32 Item) const;

	void ApplyDamageToEnemy(FMassEntityHandle Handle, int32 DamageThisHit, FVector ImpactLocation, EDamageType DamageType = EDamageType::Neutral);

	UFUNCTION(BlueprintCallable)
	void ApplyDamageToTarget(FMassEnemyTarget Target, int32 Damage, FVector ImpactLocation, EDamageType DamageType = EDamageType::Neutral);

	/** Convenience: resolves the entity from a hit result and applies damage. Returns false if the hit wasn't on a Mass enemy. */
	UFUNCTION(BlueprintCallable)
	bool ApplyDamageByHit(const FHitResult& Hit, int32 Damage, EDamageType DamageType = EDamageType::Neutral);

	UFUNCTION(BlueprintPure)
	bool IsTargetValid(FMassEnemyTarget Target) const;

	/** Returns true if the entity currently has active stealth. */
	UFUNCTION(BlueprintCallable)
	bool CheckEnemyStealth(FMassEnemyTarget Target) const;

	/** Applies a slow to the target. Amount is 0-1 slow strength (0.5 = 50% slower). Ignored if nimble. */
	UFUNCTION(BlueprintCallable)
	void ApplySlow(FMassEnemyTarget Target, float Duration, float Amount);

	/** Applies a stun to the target. Ignored if nimble. */
	UFUNCTION(BlueprintCallable)
	void ApplyStun(FMassEnemyTarget Target, float Duration);

	/** Applies a burn (DoT) to the target. TickInterval is seconds between damage ticks. Ignored if nimble. */
	UFUNCTION(BlueprintCallable)
	void ApplyBurn(FMassEnemyTarget Target, float Duration, float DamagePerTick, float TickInterval = 1.f);

	/** Adds compounding bonus damage per consecutive hit within a time window. */
	void ApplyCompoundingInjury(FMassEnemyTarget Target, float DamagePerStack, float MaxBonus, float TimeWindow);

	/** Marks the target as arc-charged. If another arc-charged enemy is already within range, arcs between them, applies electric damage to both, and clears both fragments. */
	void ApplyArcShot(FMassEnemyTarget Target, float Range, float Damage);

	/** Applies a conduit marker to the target. Immediately arcs to all other conduit-marked enemies within range. Marker persists until it expires. */
	void ApplyConduitMarker(FMassEnemyTarget Target, float Duration, float Range, float ArcDamage);

	/** Called by StatusEffectProcessor when a conduit marker fragment expires. */
	void NotifyConduitMarkerExpired(FMassEntityHandle Handle);

	FMassEntityHandle ArcChargedEntity;
	TSet<FMassEntityHandle> ConduitMarkedEntities;

	/** Tracks consecutive hits within a time window. Applies a slow when ShotThreshold is reached. Ignored if nimble. */
	void ApplySuppressed(FMassEnemyTarget Target, int32 ShotThreshold, float TimeWindow, float SlowAmount, float SlowDuration);

	/** Marks the target as ruptured — if it dies this processor frame it will explode for AoE damage. Cleared otherwise. */
	void ApplyRuptured(FMassEnemyTarget Target, float Damage, float Radius);

	/** Marks the target for Devastating Blow — if HP ratio exceeds threshold when processed, damage is multiplied. Cleared by damage processor. */
	void ApplyDevastatingBlow(FMassEnemyTarget Target, float HPThreshold, float Multiplier);

	/** Marks the target for Ballistic Recall — if it dies, restores AmmoRegain bullets to the player. */
	void ApplyBallisticRecall(FMassEnemyTarget Target, int32 AmmoRegain);

	/** Deals AoE kinetic damage to all enemies within Radius of Position. Called by the damage processor on rupture death. */
	void Rupture(FVector Position, float Damage, float Radius);

	/** Returns true if both targets reference the same Mass entity (compares Index + SerialNumber). */
	UFUNCTION(BlueprintPure)
	bool IsSameTarget(FMassEnemyTarget A, FMassEnemyTarget B) const;

	/** Returns true if the hit was on a Mass enemy, and fills OutTarget with the handle and position. */
	UFUNCTION(BlueprintCallable)
	bool GetEnemyTargetFromHit(const FHitResult& Hit, FMassEnemyTarget& OutTarget) const;

	UFUNCTION(BlueprintCallable)
	void DestroyEnemyByISMC(UInstancedStaticMeshComponent* Component, int32 Item);

	UFUNCTION(BlueprintCallable)
	bool DestroyEnemyByHit(const FHitResult& Hit);

	UFUNCTION(BlueprintCallable)
	void RewardPlayerForEnemyDeath(FResourceAmount Reward);

	UFUNCTION(BlueprintCallable)
	void AssignNiagaraComponent(UNiagaraComponent* Component);

	/** Data bundle built by HealthbarUpdateProcessor each frame and consumed by UpdateHealthbarInformation. */
	struct FHealthbarFrameData
	{
		TArray<float>            Healths;
		TArray<float>            MaxHealths;
		TArray<FVector>          Positions;
		TArray<FMassEntityHandle> Handles;
		TArray<float>            Vitalities;
		TArray<float>            BurnDurations;
		TArray<float>            SlowDurations;
		TArray<float>            StunDurations;
		TArray<float>            ModifierFlags;
		TArray<float>            FragmentedChunkSizes;
		TArray<FName>            EnemyIDs;
		float                    DeltaTime = 0.f;
	};

	void UpdateHealthbarInformation(FHealthbarFrameData& Data);

	void SpawnHitEffects(FVector Location, int32 Damage);

	/** Called by EnemyDamageMassProcessor when amorphic or fragmented mitigates damage. */
	void NotifyDamageMitigated(FMassEntityHandle Handle, float MitigatedAmount);

	/** Called by EnemyDamageMassProcessor with the final damage after all modifiers are applied. */
	void NotifyDamageDealt(FMassEntityHandle Handle, float FinalDamage);

	/** Returns all active entity handles whose position is within Radius of Center */
	void GetEntitiesInRange(FVector Center, float Radius, TArray<FMassEntityHandle>& OutHandles) const;

	/** Returns the last-known world position of the entity, or ZeroVector if not found */
	FVector GetEntityPosition(FMassEntityHandle Handle) const;

	/** Returns the current world position of the target, or ZeroVector if not found. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	FVector GetTargetPosition(FMassEnemyTarget Target) const;

	/** Returns the health ratio (0–1) of the entity, or 0 if not found */
	float GetEntityHealth(FMassEntityHandle Handle) const;

	/** Returns the EnemyID stamped on the entity at spawn, or NAME_None if not found */
	FName GetEntityEnemyID(FMassEntityHandle Handle) const;

	/** Returns true if the entity's enemy type has bIsEnhanced set in the data table. */
	bool IsEnhancedEnemy(FMassEntityHandle Handle) const;

	TArray<FMassEntityHandle> ActiveEntityHandles;

	TMap<FMassEntityHandle, int32> EntitySlotMap;
	TArray<int32> FreeSlots;

	/** Called by the damage processor when an entity's health reaches zero. Removes it from
	 *  tracking immediately and broadcasts OnEnemyDeath so towers can react this frame. */
	void NotifyEnemyDeath(FMassEntityHandle Handle);

	/** Called by the damage processor when a horde-tagged entity dies, while the entity is still valid. */
	void NotifyHordeEnemyDeath(FName HordeID);

	FOnEnemyDeathSignature OnEnemyDeath;
	FOnHordeEnemyDeathSignature OnHordeEnemyDeath;
	FOnEnemyDamageTakenSignature OnEnemyDamageTaken;

	void LoadEnemyDataFromDataTable();

	TMap<FName, FEnemyData> EnemyDataMap;

	FEnemyData GetEnemyData(const FName& EnemyID);

	// Synchronous spawning request of enemies
	void QueueSpawnRequest(const FEnemySpawnRequest& Request);
	void FlushHealthbars();
	void ProcessSpawnQueue();
	void Tick(float DeltaTime);
	TArray<FEnemySpawnRequest> PendingSpawnRequests;
	FHealthbarFrameData PendingHealthbarData;

	virtual bool IsTickable() const override
	{
		return true;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UEnemyManagerSubsystem, STATGROUP_Tickables);
	}
};
