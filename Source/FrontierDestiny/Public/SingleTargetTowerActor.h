// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerActor.h"
#include "SingleTargetTowerActor.generated.h"

class AEnemyActor;

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API ASingleTargetTowerActor : public ATowerActor
{
	GENERATED_BODY()

public:
	ASingleTargetTowerActor();

	virtual bool SelectTarget() override;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Single Target Tower Properties")
	TArray<TObjectPtr<AEnemyActor>> PotentialTargets;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Single Target Tower Properties")
	TObjectPtr<AEnemyActor> Target;

	UFUNCTION(BlueprintCallable, Category = "Single Target Tower Functions")
	bool AttackTarget();

	UFUNCTION()
	void OnTargetDeath();
};
