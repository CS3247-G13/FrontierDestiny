// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerBlueprintFunctionLibrary.h"
#include "EnemyManagerSubsystem.h"
#include "Engine/World.h"

TArray<FMassEnemyTarget> UTowerBlueprintFunctionLibrary::GetUniqueEnemyTargetsFromHits(
	const UObject* WorldContextObject,
	const TArray<FHitResult>& HitResults,
	const TArray<FMassEnemyTarget>& IgnoreTargets)
{
	TArray<FMassEnemyTarget> Result;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return Result;

	UEnemyManagerSubsystem* EnemyManager = World->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return Result;

	TSet<FMassEntityHandle> IgnoreSet;
	IgnoreSet.Reserve(IgnoreTargets.Num());
	for (const FMassEnemyTarget& Ignored : IgnoreTargets)
	{
		IgnoreSet.Add(Ignored.EntityHandle);
	}

	TSet<FMassEntityHandle> SeenHandles;
	SeenHandles.Reserve(HitResults.Num());

	for (const FHitResult& Hit : HitResults)
	{
		FMassEnemyTarget Candidate;
		if (!EnemyManager->GetEnemyTargetFromHit(Hit, Candidate)) continue;

		if (IgnoreSet.Contains(Candidate.EntityHandle)) continue;
		if (SeenHandles.Contains(Candidate.EntityHandle)) continue;

		SeenHandles.Add(Candidate.EntityHandle);
		Result.Add(Candidate);
	}

	return Result;
}

