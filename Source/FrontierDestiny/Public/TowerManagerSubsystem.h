// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TowerData.h"
#include "Upgrade.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TowerManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTowerUpgraded, FName, TowerID);

UCLASS()
class FRONTIERDESTINY_API UTowerManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void UpgradeTower(const FUpgradeData& Upgrade);

	UFUNCTION(BlueprintCallable)
	bool GetTowerBaseStats(const FName& TowerID, FTowerData& TowerData);

	UPROPERTY(VisibleAnywhere)
	TMap<FName, FTowerData> TowerDataMap;

	UPROPERTY(BlueprintAssignable)
	FOnTowerUpgraded OnTowerUpgraded;

protected:
	void LoadTowerDataFromDataTable();
};
