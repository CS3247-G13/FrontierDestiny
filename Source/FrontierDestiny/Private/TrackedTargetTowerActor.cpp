// Fill out your copyright notice in the Description page of Project Settings.

#include "TrackedTargetTowerActor.h"
#include "EnemyManagerSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

void ATrackedTargetTowerActor::ActivateTower()
{
	Super::ActivateTower();

	RangeComponent->SetGenerateOverlapEvents(true);
	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &ATrackedTargetTowerActor::OnRangeBeginOverlap);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &ATrackedTargetTowerActor::OnRangeEndOverlap);

	RangeComponent->UpdateOverlaps();

	if (UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>())
	{
		EnemyManager->OnEnemyDeath.AddUObject(this, &ATrackedTargetTowerActor::OnTrackedEnemyDied);
	}

	GetWorldTimerManager().SetTimer(
		RangeCheckTimerHandle,
		this,
		&ATrackedTargetTowerActor::CheckEnemiesInRange,
		1.0f,
		true
	);
}

void ATrackedTargetTowerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (!bIsGhost)
	{
		GetWorldTimerManager().ClearTimer(RangeCheckTimerHandle);

		if (UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>())
		{
			EnemyManager->OnEnemyDeath.RemoveAll(this);
		}
	}
}

void ATrackedTargetTowerActor::OnRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return;

	UInstancedStaticMeshComponent* ISMC = Cast<UInstancedStaticMeshComponent>(OtherComp);
	if (!ISMC) return;

	FMassEntityHandle Handle = EnemyManager->GetEnemyEntityHandle(ISMC, OtherBodyIndex);
	if (!Handle.IsSet()) return;

	FMassEnemyTarget Target;
	Target.EntityHandle = Handle;
	Target.Position = EnemyManager->GetEntityPosition(Handle);
	AddTarget(Target);
}

void ATrackedTargetTowerActor::OnRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return;

	UInstancedStaticMeshComponent* ISMC = Cast<UInstancedStaticMeshComponent>(OtherComp);
	if (!ISMC) return;

	FMassEntityHandle Handle = EnemyManager->GetEnemyEntityHandle(ISMC, OtherBodyIndex);
	if (!Handle.IsSet()) return;

	RemoveTarget(Handle);
}

void ATrackedTargetTowerActor::OnTrackedEnemyDied(FMassEntityHandle Handle)
{
	RemoveTarget(Handle);
}

void ATrackedTargetTowerActor::AddTarget(FMassEnemyTarget Target)
{
	if (OverlappingTargets.Contains(Target.EntityHandle)) return;

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager || EnemyManager->CheckEnemyStealth(Target)) return;

	OverlappingTargets.Add(Target.EntityHandle);
	OnTargetEnterRange(Target);
}

void ATrackedTargetTowerActor::RemoveTarget(FMassEntityHandle Handle)
{
	if (!OverlappingTargets.Contains(Handle)) return;

	OverlappingTargets.Remove(Handle);

	FMassEnemyTarget Target;
	Target.EntityHandle = Handle;
	OnTargetLeaveRange(Target);
}

void ATrackedTargetTowerActor::CheckEnemiesInRange()
{
	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return;

	TArray<FMassEntityHandle> NearbyEntities;
	EnemyManager->GetEntitiesInRange(GetActorLocation(), TowerData.Range, NearbyEntities);

	for (const FMassEntityHandle& Handle : NearbyEntities)
	{
		FMassEnemyTarget Target;
		Target.EntityHandle = Handle;
		Target.Position = EnemyManager->GetEntityPosition(Handle);
		AddTarget(Target);
	}
}

void ATrackedTargetTowerActor::OnTargetEnterRange_Implementation(FMassEnemyTarget Target)
{ }

void ATrackedTargetTowerActor::OnTargetLeaveRange_Implementation(FMassEnemyTarget Target)
{ }
