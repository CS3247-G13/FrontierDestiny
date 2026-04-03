// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "EnemyNavMeshGroundingProcessor.generated.h"

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API UEnemyNavMeshGroundingProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	UEnemyNavMeshGroundingProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
