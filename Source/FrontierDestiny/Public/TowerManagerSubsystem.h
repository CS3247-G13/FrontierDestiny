// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TowerData.h"
#include "Upgrade.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TowerManagerSubsystem.generated.h"

struct FTowerPathNode
{
	FName TowerID = NAME_None;

	TMap<int32, TSharedPtr<FTowerPathNode>> Children;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTowerUnlocked, FTowerData, TowerData);

UCLASS()
class FRONTIERDESTINY_API UTowerManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	bool GetTowerData(const FName& TowerID, FTowerData& TowerData);

	UFUNCTION(BlueprintPure)
	bool IsTowerUnlocked(FName TowerID) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TMap<FName, FTowerData> TowerDataMap;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TSet<FName> UnlockedTowers;

	UPROPERTY(BlueprintAssignable)
	FOnTowerUnlocked OnTowerUnlocked;

	FTowerPathNode FullTowerPath;
	bool CheckPathUnlocked(TArray<int32> Path);
	UFUNCTION(BlueprintCallable, Category = "Tower")
	TMap<int32, FTowerData> GetPathNextTowers(TArray<int32> Path);
	FTowerData GetPathTower(TArray<int32> Path);
protected:
	void LoadTowerDataFromDataTable();

	UFUNCTION()
	void HandleUpgradePerformed(const FUpgradeData& Upgrade);
};
