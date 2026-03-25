// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "EnemyDamageMassProcessor.h"

#include "HealthbarUpdateProcessor.generated.h"

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API UHealthbarUpdateProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UHealthbarUpdateProcessor();

protected:
	/** Configures the requirements (Health and Damage) for the query */
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	/** Main execution logic called every frame in the PrePhysics phase */
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	/** The query used to filter entities for processing */
	FMassEntityQuery EntityQuery;
};
