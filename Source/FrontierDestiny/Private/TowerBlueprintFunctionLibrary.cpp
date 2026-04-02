// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerBlueprintFunctionLibrary.h"
#include "CustomChannels.h"
#include "TowerUtilities.h"
#include "EnemyManagerSubsystem.h"
#include "GlobalTowerSettings.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

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

bool UTowerBlueprintFunctionLibrary::CheckEnemyVisibleFromPoint(
	const UObject* WorldContextObject,
	FVector Point,
	const FMassEnemyTarget& Target,
	const TArray<AActor*>& IgnoreActors)
{
	if (!Target.IsSet()) return false;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return false;

	UEnemyManagerSubsystem* EnemyManager = World->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return false;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = false;
	Params.AddIgnoredActors(IgnoreActors);

	const bool bHit = World->LineTraceSingleByChannel(Hit, Point, Target.Position, ECC_Visibility, Params);
	if (!bHit) return false;

	FMassEnemyTarget HitTarget;
	if (!EnemyManager->GetEnemyTargetFromHit(Hit, HitTarget)) return false;

	return EnemyManager->IsSameTarget(HitTarget, Target);
}

void UTowerBlueprintFunctionLibrary::SpawnArcLightningEffect(
	const UObject* WorldContextObject,
	FVector Origin,
	FVector Target)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return;

	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	UNiagaraSystem* ArcSystem = Settings->ArcLightningEffect.LoadSynchronous();
	if (!ArcSystem) return;

	if (UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ArcSystem, Origin))
	{
		NiagaraComp->SetVariablePosition(FName("Origin"), Origin);
		NiagaraComp->SetVariablePosition(FName("Target"), Target);
	}
}

void UTowerBlueprintFunctionLibrary::RotateTowerToFace(
	USceneComponent* TowerJoint,
	USceneComponent* TowerHead,
	FVector TargetPosition,
	float RotationSpeed,
	float DeltaTime)
{
	if (!TowerJoint || !TowerHead) return;

	// Direction from tower head to target, projected into a rotator
	const FVector Direction = (TargetPosition - TowerHead->GetComponentLocation()).GetSafeNormal();
	const FRotator ToRotator = Direction.Rotation();

	// From: current aim assembled from joint yaw + head pitch
	const FRotator FromRotator(TowerHead->GetRelativeRotation().Pitch, TowerJoint->GetComponentRotation().Yaw, 0.f);

	const FRotator Result = UTowerUtilities::QuaternionSlerp(FromRotator, ToRotator, DeltaTime, RotationSpeed);

	TowerHead->SetRelativeRotation(FRotator(Result.Pitch, 0.f, 0.f));
	TowerJoint->SetWorldRotation(FRotator(0.f, Result.Yaw, 0.f));
}
