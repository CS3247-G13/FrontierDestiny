// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerActor.h"
#include "MassEnemyTarget.h"
#include "AreaTargetTowerActor.generated.h"

/**
 * Tower that queries for targets at fire time rather than tracking them continuously.
 * Use this for towers that fire at all enemies in an area on a cooldown (e.g. AoE, chain, pulse).
 * No overlap callbacks or persistent target handles — Blueprint drives the fire loop via OnTowerTick.
 */
UCLASS()
class FRONTIERDESTINY_API AAreaTargetTowerActor : public ATowerActor
{
	GENERATED_BODY()

public:
	/** Returns up to MaxTargets enemies currently within range, excluding stealthed enemies. */
	UFUNCTION(BlueprintCallable, Category = "Tower")
	TArray<FMassEnemyTarget> SelectTargets(int32 MaxTargets) const;
};
