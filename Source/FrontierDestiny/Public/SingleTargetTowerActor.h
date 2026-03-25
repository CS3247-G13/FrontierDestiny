// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerActor.h"
#include "SingleTargetTowerActor.generated.h"

/** * Defines the logic used by the tower to prioritize targets within range.
 */
UENUM(BlueprintType)
enum class ETowerTargetingMode : uint8
{
	Nearest            UMETA(DisplayName = "Nearest"),
	Strongest          UMETA(DisplayName = "Strongest (Most Health)"),
	Weakest            UMETA(DisplayName = "Weakest (Least Health)"),
	ClosestToBase      UMETA(DisplayName = "Closest to Base (Most Progress)"),
	FurthestFromBase   UMETA(DisplayName = "Furthest from Base (Least Progress)")
};

/**
 * A specialized tower that focuses on and attacks a single enemy target within range.
 */
UCLASS()
class FRONTIERDESTINY_API ASingleTargetTowerActor : public ATowerActor
{
	GENERATED_BODY()

public:
	ASingleTargetTowerActor();

	/** Logic to evaluate and pick the best target based on the current TargetingMode */
	UFUNCTION(BlueprintCallable)
	void SelectTarget();

	virtual void ActivateTower() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void OnTargetLeaveRange_Implementation(FMassEnemyTarget Target) override;

protected:

	/** The current priority logic for picking targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Single Target Tower Properties")
	ETowerTargetingMode TargetingMode;

	/** The currently tracked Mass entity target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Single Target Tower Properties")
	FMassEnemyTarget CurrentTarget;

	/** Called when the current target entity is no longer valid (destroyed) */
	UFUNCTION(BlueprintNativeEvent)
	void OnTargetDeath();

	/** Return false to trigger LoseSightOfTarget; override in BP for line-trace etc. */
	UFUNCTION(BlueprintNativeEvent)
	bool CheckTargetVisible(FMassEnemyTarget Target);

	UFUNCTION(BlueprintNativeEvent)
	void LoseSightOfTarget();
	FTimerHandle TargetCheckTimer;

	UFUNCTION()
	void PerformCurrentTargetVisibilityCheck();
};