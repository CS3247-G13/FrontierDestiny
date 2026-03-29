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

	for (const FHitResult& Hit : HitResults)
	{
		FMassEnemyTarget Candidate;
		if (!EnemyManager->GetEnemyTargetFromHit(Hit, Candidate)) continue;

		bool bAlreadyPresent = false;
		for (const FMassEnemyTarget& Existing : Result)
		{
			if (EnemyManager->IsSameTarget(Existing, Candidate))
			{
				bAlreadyPresent = true;
				break;
			}
		}

		if (!bAlreadyPresent)
		{
			for (const FMassEnemyTarget& Ignored : IgnoreTargets)
			{
				if (EnemyManager->IsSameTarget(Ignored, Candidate))
				{
					bAlreadyPresent = true;
					break;
				}
			}
		}

		if (!bAlreadyPresent)
		{
			Result.Add(Candidate);
		}
	}

	return Result;
}

