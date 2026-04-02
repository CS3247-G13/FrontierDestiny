// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerActor.h"
#include "MassEnemyTarget.h"
#include "ProximityTowerActor.generated.h"

/**
 * Tower that fires a Blueprint event the moment an enemy enters its range.
 * No target list is maintained — each entry triggers independently.
 */
UCLASS()
class FRONTIERDESTINY_API AProximityTowerActor : public ATowerActor
{
	GENERATED_BODY()

protected:
	virtual void ActivateTower() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Tower Functions")
	void OnProximityTriggered(FMassEnemyTarget Target);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void CheckEnemiesInRange();

	FTimerHandle RangeCheckTimerHandle;
};
