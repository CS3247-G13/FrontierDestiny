// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TowerData.generated.h"

/**
 * 
 */

class ATowerActor;

UCLASS()
class FRONTIERDESTINY_API UTowerData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 2D Thumbnail icon for the tower
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tower")
    UTexture2D* TowerIcon;

    // The blueprint of the tower
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tower")
    TSubclassOf<ATowerActor> TowerBlueprint;
    
	// The cell relative to the origin that serves as the pivot for placement
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
    FIntPoint PivotPoint;

    // The cells this tower occupies relative to the pivot (0,0)
    // Example for 2x2: (0,0), (1,0), (0,1), (1,1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
    TArray<FIntPoint> Footprint;

	// The cost to build the tower
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
	int32 TowerCost;

    // The upgraded version of this tower, if any
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades")
	TArray<UTowerData*> AvailableUpgrades;
};