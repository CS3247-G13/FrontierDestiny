// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnemyData.h"
#include "CoreMinimal.h"
#include "MassEnemyTarget.h"
#include "NiagaraComponent.h"
#include "MassEntityHandle.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "EnemyManagerSubsystem.generated.h"

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
	TArray<float> HealthRatios;
	TArray<FVector> EnemyPositions;
	TArray<float> EnemyVisibilities;
	void InitializeHealthbars();

	FMassEntityHandle GetEnemyEntityHandle(UInstancedStaticMeshComponent* Component, int32 Item) const;

	void ApplyDamageToEnemy(FMassEntityHandle Handle, int DamageThisHit);

	UFUNCTION(BlueprintCallable)
	void ApplyDamageToTarget(FMassEnemyTarget Target, int32 Damage);

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

	void UpdateHealthbarInformation(TArray<float>& UpdatedHealthRatios, TArray<FVector>& UpdatedEnemyPositions, TArray<FMassEntityHandle>& UpdatedEntityHandles);

	/** Returns all active entity handles whose position is within Radius of Center */
	void GetEntitiesInRange(FVector Center, float Radius, TArray<FMassEntityHandle>& OutHandles) const;

	/** Returns the last-known world position of the entity, or ZeroVector if not found */
	FVector GetEntityPosition(FMassEntityHandle Handle) const;

	/** Returns the health ratio (0–1) of the entity, or 0 if not found */
	float GetEntityHealth(FMassEntityHandle Handle) const;

	TArray<FMassEntityHandle> ActiveEntityHandles;

	void LoadEnemyDataFromDataTable();

	TMap<FName, FEnemyData> EnemyDataMap;

	FEnemyData GetEnemyData(const FName& EnemyID);

	
};
