// Fill out your copyright notice in the Description page of Project Settings.

#include "ProximityTowerActor.h"
#include "EnemyManagerSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

void AProximityTowerActor::ActivateTower()
{
	Super::ActivateTower();

	RangeComponent->SetGenerateOverlapEvents(true);
	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &AProximityTowerActor::OnRangeBeginOverlap);
	RangeComponent->UpdateOverlaps();

	GetWorldTimerManager().SetTimer(
		RangeCheckTimerHandle,
		this,
		&AProximityTowerActor::CheckEnemiesInRange,
		1.0f,
		true
	);
}

void AProximityTowerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (!bIsGhost)
	{
		GetWorldTimerManager().ClearTimer(RangeCheckTimerHandle);
	}
}

void AProximityTowerActor::OnRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

	OnProximityTriggered(Target);
}

void AProximityTowerActor::CheckEnemiesInRange()
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
		OnProximityTriggered(Target);
	}
}

void AProximityTowerActor::OnProximityTriggered_Implementation(FMassEnemyTarget Target)
{ }
