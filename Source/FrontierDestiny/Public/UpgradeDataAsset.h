// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TowerActor.h"

#include "TowerData.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UpgradeDataAsset.generated.h"

UCLASS()
class FRONTIERDESTINY_API UUpgradeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	TObjectPtr<ATowerActor> UpgradeTarget;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FText UpgradeName;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FText UpgradeDescription;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FName UpgradeID;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	TObjectPtr<UTexture2D> UpgradeIcon;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Upgrade")
	FTowerData UpgradeStats;
};
