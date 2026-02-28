// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TowerData.h"
#include "Upgrade.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TowerManagerSubsystem.generated.h"

UCLASS()
class FRONTIERDESTINY_API UTowerManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	bool GetTowerData(const FName& TowerID, FTowerData& TowerData);

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TMap<FName, FTowerData> TowerDataMap;

protected:
	void LoadTowerDataFromDataTable();
};
