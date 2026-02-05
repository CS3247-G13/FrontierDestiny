// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerActor.h"
#include "SingleTargetTowerActor.generated.h"

class AEnemyActor;
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

	virtual void BeginPlay() override;

	/** Logic to evaluate and pick the best target based on the current TargetingMode */
	virtual void SelectTarget() override;

protected:

	/** The collision sphere used to detect enemies in range */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> RangeSphere;

	/** The current priority logic for picking targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Single Target Tower Properties")
	ETowerTargetingMode TargetingMode;

	// This tower targets enemies
	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Single Target Tower Properties")
	TObjectPtr<AEnemyActor> CurrentTarget;

	UFUNCTION()
	void OnTargetDeath();

};