// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnemyData.h"
#include "CoreMinimal.h"
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

	FMassEntityHandle GetEnemyEntityHandle(UInstancedStaticMeshComponent* Component, int32 Item);

	void ApplyDamageToEnemy(FMassEntityHandle Handle, int DamageThisHit);

	UFUNCTION(BlueprintCallable)
	void DestroyEnemyByISMC(UInstancedStaticMeshComponent* Component, int32 Item);

	UFUNCTION(BlueprintCallable)
	void AssignNiagaraComponent(UNiagaraComponent* Component);

	void UpdateHealthbarInformation(TArray<float>& UpdatedHealthRatios, TArray<FVector>& UpdatedEnemyPositions);

	void LoadEnemyDataFromDataTable();

	TMap<FName, FEnemyData> EnemyDataMap;

	FEnemyData GetEnemyData(const FName& EnemyID);

	
};
