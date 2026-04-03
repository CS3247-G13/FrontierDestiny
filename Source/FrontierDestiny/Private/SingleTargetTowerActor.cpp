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
		true
	);

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (EnemyManager)
	{
		EnemyDeathDelegateHandle = EnemyManager->OnEnemyDeath.AddUObject(this, &ASingleTargetTowerActor::HandleEnemyDeath);
	}
}

void ASingleTargetTowerActor::EndPlay(const EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);
	GetWorldTimerManager().ClearTimer(TargetCheckTimer);

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (EnemyManager)
	{
		EnemyManager->OnEnemyDeath.Remove(EnemyDeathDelegateHandle);
	}
}

void ASingleTargetTowerActor::HandleEnemyDeath(FMassEntityHandle Handle)
{
	// Always remove the dead entity from range tracking so it can't be retargeted
	OverlappingTargets.Remove(Handle);

	if (CurrentTarget.EntityHandle == Handle)
	{
		CurrentTarget = FMassEnemyTarget();
		OnTargetDeath();
	}
}

void ASingleTargetTowerActor::PerformCurrentTargetVisibilityCheck()
{
	if (!CurrentTarget.IsSet())
	{
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

	// Score all candidates — no visibility checks yet (potentially expensive)
	struct FScoredHandle
	{
		FMassEntityHandle Handle;
		float Score;
	};
	TArray<FScoredHandle> Scored;
	Scored.Reserve(OverlappingTargets.Num());

	for (const FMassEntityHandle& Handle : OverlappingTargets)
	{
		if (!Handle.IsSet()) continue;

		const FVector EnemyPos = EnemyManager->GetEntityPosition(Handle);
		const float Health = EnemyManager->GetEntityHealth(Handle);
		float Score = 0.f;

		switch (TargetingMode)
		{
		case ETowerTargetingMode::Nearest:
			Score = -FVector::DistSquared(EnemyPos, GetActorLocation());
			break;
		case ETowerTargetingMode::Strongest:
			Score = Health;
			break;
		case ETowerTargetingMode::Weakest:
			Score = -Health;
			break;
		case ETowerTargetingMode::ClosestToBase:
		case ETowerTargetingMode::FurthestFromBase:
			break;
		}

		Scored.Add({ Handle, Score });
	}

	Scored.Sort([](const FScoredHandle& A, const FScoredHandle& B) { return A.Score > B.Score; });

	// Walk best-to-worst, stop at the first visible candidate
	for (const FScoredHandle& Entry : Scored)
	{
		FMassEnemyTarget Candidate;
		Candidate.EntityHandle = Entry.Handle;
		Candidate.Position = EnemyManager->GetEntityPosition(Entry.Handle);

		if (!CheckTargetVisible(Candidate)) continue;

		BestHandle = Entry.Handle;
		break;
	}

	if (BestHandle.IsSet())
	{
		FMassEnemyTarget NewTarget;
		NewTarget.EntityHandle = BestHandle;
		NewTarget.Position = EnemyManager->GetEntityPosition(BestHandle);

		if (CurrentTarget.EntityHandle != BestHandle)
		{
			CurrentTarget = NewTarget;
			OnAcquireNewTarget(CurrentTarget);
		}
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
	SelectTarget();
}

void ASingleTargetTowerActor::OnAcquireNewTarget_Implementation(FMassEnemyTarget Target)
{
}
