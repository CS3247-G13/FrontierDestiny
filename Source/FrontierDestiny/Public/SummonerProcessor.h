// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SummonerProcessor.generated.h"


/** Indicates a Summoner unit */
USTRUCT()
struct FSummonerFragment : public FMassFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float Cooldown = 5.f;
	UPROPERTY(EditAnywhere) float TimeRemaining = 5.f;
	UPROPERTY(EditAnywhere) int32 UnitsPerSummon = 2;
	UPROPERTY(EditAnywhere) float SpawnRadius = 300.f;
	UPROPERTY(EditAnywhere) FName EnemyID; //EnemyID of spawned unit
};

USTRUCT()
struct FSummonedByFragment : public FMassFragment
{
	GENERATED_BODY()
	FMassEntityHandle Owner;
};

USTRUCT()
struct FSummonRequestFragment : public FMassFragment
{
	GENERATED_BODY()
	int32 Count = 0;
	float Radius = 0.f;
	FName EnemyID;
};

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API USummonerProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	USummonerProcessor();

protected:
	/** Configures the requirements (SummonerFragment) for the query */
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	/** Main execution logic called every frame in the PrePhysics phase */
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	/** The query used to filter entities for processing */
	FMassEntityQuery EntityQuery;

	/** Second Query because lazy to add another processor (for now) */
	FMassEntityQuery EQ2;
};
