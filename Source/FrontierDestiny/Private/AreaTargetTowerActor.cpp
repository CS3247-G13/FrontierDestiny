// Fill out your copyright notice in the Description page of Project Settings.

#include "AreaTargetTowerActor.h"
#include "EnemyManagerSubsystem.h"
#include "TowerBlueprintFunctionLibrary.h"
#include "Components/SphereComponent.h"

TArray<FMassEnemyTarget> AAreaTargetTowerActor::SelectTargets(int32 MaxTargets) const
{
	TArray<FMassEnemyTarget> Result;

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return Result;

	TArray<FMassEntityHandle> Handles;
	EnemyManager->GetEntitiesInRange(RangeComponent->GetComponentLocation(), TowerData.Range, Handles);

	const FVector TraceOrigin = RangeComponent->GetComponentLocation();
	Handles.Sort([&](const FMassEntityHandle& A, const FMassEntityHandle& B)
	{
		const float DistA = FVector::DistSquared(EnemyManager->GetEntityPosition(A), TraceOrigin);
		const float DistB = FVector::DistSquared(EnemyManager->GetEntityPosition(B), TraceOrigin);
		return DistA < DistB;
	});

	TArray<AActor*> Ignored;
	Ignored.Add(const_cast<AAreaTargetTowerActor*>(this));

	for (const FMassEntityHandle& Handle : Handles)
	{
		if (Result.Num() >= MaxTargets) break;

		FMassEnemyTarget Target;
		Target.EntityHandle = Handle;
		Target.Position = EnemyManager->GetEntityPosition(Handle);

		if (EnemyManager->CheckEnemyStealth(Target)) continue;
		if (!UTowerBlueprintFunctionLibrary::CheckEnemyVisibleFromPoint(this, TraceOrigin, Target, Ignored)) continue;

		Result.Add(Target);
	}

	return Result;
}
