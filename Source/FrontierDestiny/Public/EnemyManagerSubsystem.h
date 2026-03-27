// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnemyData.h"
#include "CoreMinimal.h"
#include "MassEnemyTarget.h"
#include "NiagaraComponent.h"
#include "MassEntityHandle.h"
#include "EnemyDamageMassProcessor.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "EnemyManagerSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyDeathSignature, FMassEntityHandle);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHordeEnemyDeathSignature, FName);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnemyDamageTakenSignature, FVector, int32);

UCLASS()
class FRONTIERDESTINY_API UEnemyManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Healthbars
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	// Per-slot arrays pushed to Niagara every frame
	TArray<float> HealthRatios;
	TArray<FVector> EnemyPositions;
	TArray<float> EnemyVisibilities;
	TArray<float> VitalityRatios;
	TArray<float> BurnDurations;
	TArray<float> SlowDurations;
	TArray<float> StunDurations;
	TArray<float> ModifierFlags;      // booleans packed as int cast to float
	TArray<float> SplitRatios;        // where the upper/lower healthbar split is (0 = none)
	TArray<float> SplitEffects;       // which visual for the upper portion
	TArray<float> MitigatedDamageAmounts; // fading mitigated damage bar (amorphic + fragmented)

	// Internal fade state — not pushed to Niagara
	TArray<float> MitigatedDamageFadeTimers;

	void InitializeHealthbars();

	FMassEntityHandle GetEnemyEntityHandle(UInstancedStaticMeshComponent* Component, int32 Item) const;

	void ApplyDamageToEnemy(FMassEntityHandle Handle, int32 DamageThisHit, FVector ImpactLocation, EDamageType DamageType = EDamageType::None);

	UFUNCTION(BlueprintCallable)
	void ApplyDamageToTarget(FMassEnemyTarget Target, int32 Damage, FVector ImpactLocation);

	/** Convenience: resolves the entity from a hit result and applies damage. Returns false if the hit wasn't on a Mass enemy. */
	UFUNCTION(BlueprintCallable)
	bool ApplyDamageByHit(const FHitResult& Hit, int32 Damage);

	UFUNCTION(BlueprintPure)
	bool IsTargetValid(FMassEnemyTarget Target) const;

	/** Returns true if both targets reference the same Mass entity (compares Index + SerialNumber). */
	UFUNCTION(BlueprintPure)
	bool IsSameTarget(FMassEnemyTarget A, FMassEnemyTarget B) const;

	/** Returns true if the hit was on a Mass enemy, and fills OutTarget with the handle and position. */
	UFUNCTION(BlueprintCallable)
	bool GetEnemyTargetFromHit(const FHitResult& Hit, FMassEnemyTarget& OutTarget) const;

	UFUNCTION(BlueprintCallable)
	void DestroyEnemyByISMC(UInstancedStaticMeshComponent* Component, int32 Item);

	UFUNCTION(BlueprintCallable)
	void AssignNiagaraComponent(UNiagaraComponent* Component);

	/** Data bundle built by HealthbarUpdateProcessor each frame and consumed by UpdateHealthbarInformation. */
	struct FHealthbarFrameData
	{
		TArray<float>            HealthRatios;
		TArray<FVector>          Positions;
		TArray<FMassEntityHandle> Handles;
		TArray<float>            VitalityRatios;
		TArray<float>            BurnDurations;
		TArray<float>            SlowDurations;
		TArray<float>            StunDurations;
		TArray<float>            ModifierFlags;
		TArray<float>            SplitRatios;
		TArray<float>            SplitEffects;
		float                    DeltaTime = 0.f;
	};

	void UpdateHealthbarInformation(FHealthbarFrameData& Data);

	void SpawnHitEffects(FVector Location, int32 Damage);

	/** Called by EnemyDamageMassProcessor when amorphic or fragmented mitigates damage. */
	void NotifyDamageMitigated(FMassEntityHandle Handle, float MitigatedAmount);

	/** Returns all active entity handles whose position is within Radius of Center */
	void GetEntitiesInRange(FVector Center, float Radius, TArray<FMassEntityHandle>& OutHandles) const;

	/** Returns the last-known world position of the entity, or ZeroVector if not found */
	FVector GetEntityPosition(FMassEntityHandle Handle) const;

	/** Returns the health ratio (0–1) of the entity, or 0 if not found */
	float GetEntityHealth(FMassEntityHandle Handle) const;

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

};
