// Fill out your copyright notice in the Description page of Project Settings.

#include "SingleTargetTowerActor.h"
#include "EnemyManagerSubsystem.h"
#include "MassEntitySubsystem.h"

ASingleTargetTowerActor::ASingleTargetTowerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	TargetingMode = ETowerTargetingMode::Nearest;
}

void ASingleTargetTowerActor::ActivateTower()
{
	Super::ActivateTower();

	GetWorldTimerManager().SetTimer(
		TargetCheckTimer,
		this,
		&ASingleTargetTowerActor::PerformCurrentTargetVisibilityCheck,
		0.2f,
		true  // loop — we need to continuously detect entity death and update position
	);
}

void ASingleTargetTowerActor::EndPlay(const EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);
	GetWorldTimerManager().ClearTimer(TargetCheckTimer);
}

void ASingleTargetTowerActor::PerformCurrentTargetVisibilityCheck()
{
	if (!CurrentTarget.IsSet())
	{
		return;
	}

	// Check if the entity has been destroyed
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem || !EntitySubsystem->GetEntityManager().IsEntityValid(CurrentTarget.EntityHandle))
	{
		OnTargetDeath();
		return;
	}

	// Keep position up-to-date
	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (EnemyManager)
	{
		CurrentTarget.Position = EnemyManager->GetEntityPosition(CurrentTarget.EntityHandle);
	}

	if (!CheckTargetVisible(CurrentTarget))
	{
		LoseSightOfTarget();
	}
}

void ASingleTargetTowerActor::SelectTarget()
{
	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return;

	FMassEntityHandle BestHandle;
	float MinValue = TNumericLimits<float>::Max();
	float MaxValue = -TNumericLimits<float>::Max();

	for (const FMassEntityHandle& Handle : OverlappingTargets)
	{
		if (!Handle.IsSet()) continue;

		const FVector EnemyPos = EnemyManager->GetEntityPosition(Handle);
		const float Health = EnemyManager->GetEntityHealth(Handle);
		const float DistanceToTower = FVector::Distance(EnemyPos, GetActorLocation());

		switch (TargetingMode)
		{
		case ETowerTargetingMode::Nearest:
			if (DistanceToTower < MinValue) { MinValue = DistanceToTower; BestHandle = Handle; }
			break;

		case ETowerTargetingMode::Strongest:
			if (Health > MaxValue) { MaxValue = Health; BestHandle = Handle; }
			break;

		case ETowerTargetingMode::Weakest:
			if (Health < MinValue) { MinValue = Health; BestHandle = Handle; }
			break;

		case ETowerTargetingMode::ClosestToBase:
		case ETowerTargetingMode::FurthestFromBase:
			// Progress tracking not yet implemented
			break;
		}
	}

	if (BestHandle.IsSet())
	{
		CurrentTarget.EntityHandle = BestHandle;
		CurrentTarget.Position = EnemyManager->GetEntityPosition(BestHandle);
	}
	else
	{
		CurrentTarget = FMassEnemyTarget();
	}
}

void ASingleTargetTowerActor::OnTargetLeaveRange_Implementation(FMassEnemyTarget Target)
{
	if (CurrentTarget.EntityHandle == Target.EntityHandle)
	{
		CurrentTarget = FMassEnemyTarget();
		SelectTarget();
	}
}

bool ASingleTargetTowerActor::CheckTargetVisible_Implementation(FMassEnemyTarget Target)
{
	return true;
}

void ASingleTargetTowerActor::LoseSightOfTarget_Implementation()
{
	CurrentTarget = FMassEnemyTarget();
	SelectTarget();
}

void ASingleTargetTowerActor::OnTargetDeath_Implementation()
{
	CurrentTarget = FMassEnemyTarget();
	SelectTarget();
}
