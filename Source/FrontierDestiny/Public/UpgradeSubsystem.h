// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Upgrade.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UpgradeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPrerequisiteMet, FName, PrerequisiteID, FName, UnlockID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradePerformed, const FUpgradeData&, Upgrade);

UCLASS()
class FRONTIERDESTINY_API UUpgradeManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void LoadUpgradesFromDataTable();

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool PerformUpgrade(const FName& UpgradeID);

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool GetUpgradeData(const FName& UpgradeID, FUpgradeData& Upgrade) const;

	TMap<FName, FUpgradeData> UpgradeMap;

	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnPrerequisiteMet OnPrerequisiteMet;
	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnUpgradePerformed OnUpgradePerformed;
};
