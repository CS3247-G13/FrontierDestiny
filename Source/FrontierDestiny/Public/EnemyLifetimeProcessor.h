// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "EnemyLifetimeProcessor.generated.h"

/**
 * Ticks down FLifetimeFragment.RemainingTime each frame.
 * When it reaches zero the entity's health is set to 0 and a lethal FDamageFragment is added,
 * routing the kill through the normal EnemyDamageMassProcessor death path.
 */
UCLASS()
class FRONTIERDESTINY_API UEnemyLifetimeProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UEnemyLifetimeProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery LifetimeQuery;
};
