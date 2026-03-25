
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "EnemyDamageMassProcessor.generated.h"


/** Fragment to store an entity's current health */

USTRUCT()
struct FHealthFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Mass")
	float Value = 20.f;
	UPROPERTY(EditAnywhere, Category = "Mass")
	float MaxValue = 20.f;
};


/** Fragment to indicate pending damage for an entity */
USTRUCT()
struct FDamageFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Mass")
	float DamageAmount = 10.f;
};

/**
 * Processor that iterates over entities with both Health and Damage fragments.
 * It applies damage and destroys the entity if health reaches zero.
 */
UCLASS()
class FRONTIERDESTINY_API UEnemyDamageMassProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UEnemyDamageMassProcessor();

protected:
	/** Configures the requirements (Health and Damage) for the query */
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	/** Main execution logic called every frame in the PrePhysics phase */
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	/** The query used to filter entities for processing */
	FMassEntityQuery EntityQuery;
};