// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "StatusEffectProcessor.generated.h"

/**
 * Ticks down burn/slow/stun durations and removes expired fragments.
 * Also updates FStatsFragment.EffectiveSpeed based on active slow/stun.
 *
 * TODO: Hook EffectiveSpeed into the Mass movement system (FMassMoveTargetFragment.DesiredSpeed)
 * once the nav/movement processor execution order is confirmed.
 */
UCLASS()
class FRONTIERDESTINY_API UStatusEffectProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UStatusEffectProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery SpeedEffectQuery;
	FMassEntityQuery BurnQuery;
};
