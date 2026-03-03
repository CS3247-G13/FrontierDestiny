// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerActor.h"
#include "SingleTargetTowerActor.generated.h"

class ABaseEnemyCharacter;
class USphereComponent;

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

protected:

	/** The current priority logic for picking targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Single Target Tower Properties")
	ETowerTargetingMode TargetingMode;

	// This tower targets enemies
	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Single Target Tower Properties")
	TObjectPtr<ABaseEnemyCharacter> CurrentTarget;

	UFUNCTION(BlueprintNativeEvent)
	void OnTargetDeath();

	UFUNCTION(BlueprintNativeEvent)
	bool CheckTargetVisible(ABaseEnemyCharacter* Target);

	UFUNCTION(BlueprintNativeEvent)
	void LoseSightOfTarget();
	FTimerHandle TargetCheckTimer;

	UFUNCTION()
	void PerformCurrentTargetVisibilityCheck();
};