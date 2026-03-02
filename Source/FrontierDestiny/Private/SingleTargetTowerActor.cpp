// Fill out your copyright notice in the Description page of Project Settings.

#include "SingleTargetTowerActor.h"
#include "BaseEnemyCharacter.h"
#include "StatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"

ASingleTargetTowerActor::ASingleTargetTowerActor()
{
	// No longer creating RangeSphere here; it's inherited from ATowerActor

	// Defaults
	PrimaryActorTick.bCanEverTick = true;
	TargetingMode = ETowerTargetingMode::Nearest;
	TargetClassFilter = ABaseEnemyCharacter::StaticClass();
}

void ASingleTargetTowerActor::SelectTarget()
{
	ABaseEnemyCharacter* BestEnemy = nullptr;
	float MinValue = TNumericLimits<float>::Max();
	float MaxValue = -TNumericLimits<float>::Max();

	// 1. Cleanup and Evaluate inherited list
	for (AActor*& Target : OverlappingTargets)
	{
		ABaseEnemyCharacter* Enemy = Cast<ABaseEnemyCharacter>(Target);

		if (!IsValid(Enemy) || Enemy->StatComponent->GetStat(TEXT("HP")) <= 0 || !CanHitTarget(Enemy))
		{
			// Removing it here will cause issues, just let the overlap check
			// handle it
			continue;
		}

		float DistanceToTower = FVector::Distance(Enemy->GetActorLocation(), GetActorLocation());
		float Health = Enemy->StatComponent->GetStat(TEXT("HP"));
		float Progress = 0.0f; // To be implemented in Enemy representing distance to base

		switch (TargetingMode)
		{
		case ETowerTargetingMode::Nearest:
			if (DistanceToTower < MinValue)
			{
				MinValue = DistanceToTower;
				BestEnemy = Enemy;
			}
			break;

		case ETowerTargetingMode::Strongest:
			if (Health > MaxValue)
			{
				MaxValue = Health;
				BestEnemy = Enemy;
			}
			break;

		case ETowerTargetingMode::Weakest:
			if (Health < MinValue)
			{
				MinValue = Health;
				BestEnemy = Enemy;
			}
			break;

		case ETowerTargetingMode::ClosestToBase:
			if (Progress > MaxValue)
			{
				MaxValue = Progress;
				BestEnemy = Enemy;
			}
			break;

		case ETowerTargetingMode::FurthestFromBase:
			if (Progress < MinValue)
			{
				MinValue = Progress;
				BestEnemy = Enemy;
			}
			break;
		}
	}

	// 2. Finalize selection (CurrentTarget is inherited)
	if (BestEnemy != nullptr)
	{
		if (CurrentTarget == BestEnemy) return;

		// Cast current target to handle enemy-specific delegate cleanup
		if (ABaseEnemyCharacter* OldEnemy = Cast<ABaseEnemyCharacter>(CurrentTarget))
		{
			OldEnemy->OnDeath.RemoveDynamic(this, &ASingleTargetTowerActor::OnTargetDeath);
		}

		CurrentTarget = BestEnemy;
		BestEnemy->OnDeath.AddDynamic(this, &ASingleTargetTowerActor::OnTargetDeath);
		return;
	}

	CurrentTarget = nullptr;
}

void ASingleTargetTowerActor::OnTargetEnterOrLeaveRange()
{
	if (!IsValid(CurrentTarget))
	{
		SelectTarget();
	}
}

void ASingleTargetTowerActor::OnTargetDeath()
{
	// CurrentTarget is cleared; the base class will handle list updates on next refresh or overlap end
	CurrentTarget = nullptr;
	SelectTarget();
}