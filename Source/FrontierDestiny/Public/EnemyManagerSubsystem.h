// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnemyData.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EnemyManagerSubsystem.generated.h"

UCLASS()
class FRONTIERDESTINY_API UEnemyManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void LoadEnemyDataFromDataTable();

	TMap<FName, FEnemyData> EnemyDataMap;

	FEnemyData GetEnemyData(const FName& EnemyID);
};
