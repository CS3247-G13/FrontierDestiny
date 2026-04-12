// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PlayerData.h"
#include "Upgrade.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerStatsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRestoreBullets, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxBulletsChanged, int32, MaxBullets);

UCLASS()
class FRONTIERDESTINY_API UPlayerManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	FPlayerData PlayerData;

	UFUNCTION()
	void HandleUpgradePerformed(const FUpgradeData& Upgrade);

	UFUNCTION()
	void HandleUpgradeActivated(const FUpgradeData& Upgrade);

	UFUNCTION()
	void HandleUpgradeDeactivated(const FUpgradeData& Upgrade);

	UPROPERTY(BlueprintAssignable)
	FOnPlayerStatsChanged OnPlayerStatsChanged;

	UPROPERTY(BlueprintAssignable)
	FOnRestoreBullets OnRestoreBullets;

	UPROPERTY(BlueprintAssignable)
	FOnMaxBulletsChanged OnMaxBulletsChanged;

	void RestoreBullets(int32 Amount);

	UFUNCTION(BlueprintCallable)
	void OnLevelChanged(UWorld* World);

protected:
	void LoadPlayerDataFromDataTable();

private:
	void SetUpgradeBoolean(const FName& TargetID, bool bValue);
};
