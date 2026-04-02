// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerActor.h"
#include "MassEnemyTarget.h"
#include "MassEntityHandle.h"
#include "TrackedTargetTowerActor.generated.h"

/**
 * Tower that maintains a persistent set of enemies in range.
 * Use this for towers that need to react to enemies entering/leaving range (e.g. single-target, beam).
 */
UCLASS()
class FRONTIERDESTINY_API ATrackedTargetTowerActor : public ATowerActor
{
	GENERATED_BODY()

protected:
	virtual void ActivateTower() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Called when a Mass entity enters the range of the tower. */
	UFUNCTION(BlueprintNativeEvent, Category = "Tower Functions")
	void OnTargetEnterRange(FMassEnemyTarget Target);

	/** Called when a Mass entity leaves the range of the tower. */
	UFUNCTION(BlueprintNativeEvent, Category = "Tower Functions")
	void OnTargetLeaveRange(FMassEnemyTarget Target);

	TSet<FMassEntityHandle> OverlappingTargets;

private:
	UFUNCTION()
	void OnRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnTrackedEnemyDied(FMassEntityHandle Handle);
	void AddTarget(FMassEnemyTarget Target);
	void RemoveTarget(FMassEntityHandle Handle);
	void CheckEnemiesInRange();

	FTimerHandle RangeCheckTimerHandle;
};
