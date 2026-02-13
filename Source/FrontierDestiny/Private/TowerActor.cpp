// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerActor.h"
#include "GlobalTowerSettings.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "TowerData.h"

ATowerActor::ATowerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetupAttachment(RootComponent);

	// Default target filter to any actor
	TargetClassFilter = AActor::StaticClass();

	// Initial detection settings in constructor
	RangeComponent->SetSphereRadius(TowerRange);

	// Change to overlap all channels
	RangeComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	RangeComponent->SetGenerateOverlapEvents(true);
	
	OverlapCheckInterval = 0.2f;
}

void ATowerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Update sphere radius in the editor when TowerRange is changed
	if (RangeComponent)
	{
		RangeComponent->SetSphereRadius(TowerRange);
	}
}

void ATowerActor::InitializeGhostTower()
{
	SetActorTickEnabled(false);
	RangeComponent->SetGenerateOverlapEvents(false);
	UpdateGhostMaterials();

	TArray<UPrimitiveComponent*> Components;
	GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* PrimComp : Components)
	{
		if (PrimComp)
		{
			// This stops overlaps, blocks, and raycasts (line traces)
			PrimComp->SetCollisionResponseToAllChannels(ECR_Ignore);
			PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PrimComp->SetGenerateOverlapEvents(false);
		}
	}
}

void ATowerActor::InitializeTower()
{
	GetWorldTimerManager().SetTimer(
		BuildTimerHandle,
		this,
		&ATowerActor::ActivateTower,
		2.0f,
		false
	);
}

void ATowerActor::ActivateTower()
{
	bIsBuilding = false;
	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &ATowerActor::OnRangeBeginOverlap);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &ATowerActor::OnRangeEndOverlap);

	FTimerManagerTimerParameters TimerParams;
	TimerParams.bLoop = true;
	TimerParams.bMaxOncePerFrame = true;
	TimerParams.FirstDelay = -1.f;
	// Optimization: Instead of checking every tick, we check every X seconds
	GetWorldTimerManager().SetTimer(OverlapCheckTimerHandle, this, &ATowerActor::CheckAllOverlaps, OverlapCheckInterval, TimerParams);

	OnTowerActive();
}

void ATowerActor::BeginPlay()
{
	Super::BeginPlay();

	// Final sync of radius at runtime
	RangeComponent->SetSphereRadius(TowerRange);

	if (bIsGhost)
	{
		InitializeGhostTower();
	}
	else
	{
		bIsBuilding = true;
		InitializeTower();
	}
}

void ATowerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (!bIsGhost)
	{
		GetWorldTimerManager().ClearTimer(OverlapCheckTimerHandle);
		GetWorldTimerManager().ClearTimer(BuildTimerHandle);
	}
}

void ATowerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsGhost || bIsBuilding)
	{
		return;
	}

	OnTowerTick(DeltaSeconds);
}

void ATowerActor::SetGhostValidity(bool bNewIsValid)
{
	if (bIsGhost && bIsValidGhost != bNewIsValid)
	{
		bIsValidGhost = bNewIsValid;
		UpdateGhostMaterials();
	}
}

void ATowerActor::UpdateGhostMaterials()
{
	if (!bIsGhost) return;

	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();
	if (!Settings) return;

	TSoftObjectPtr<UMaterialInterface> MaterialPtr = bIsValidGhost ? Settings->GhostMaterialValid : Settings->GhostMaterialInvalid;
	UMaterialInterface* TargetMat = MaterialPtr.LoadSynchronous();

	if (!TargetMat) return;

	TArray<UPrimitiveComponent*> Components;
	GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* PrimComp : Components)
	{
		if (PrimComp == RangeComponent) continue;

		for (int32 i = 0; i < PrimComp->GetNumMaterials(); ++i)
		{
			PrimComp->SetMaterial(i, TargetMat);
		}
	}
}

void ATowerActor::SelectTarget()
{
	// Default implementation: no target selection logic
}

TArray<UTowerData*> ATowerActor::GetUpgrades()
{
	return TowerInfo->AvailableUpgrades;
}

void ATowerActor::OnRangeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if the actor is not a ghost, is valid, and matches our class filter
	if (!bIsGhost && OtherActor && OtherActor != this && OtherActor->IsA(TargetClassFilter))
	{
		OverlappingTargets.AddUnique(OtherActor);
		SelectTarget();
	}
}

void ATowerActor::OnRangeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		OverlappingTargets.Remove(OtherActor);
		SelectTarget();
	}
}

void ATowerActor::CheckAllOverlaps()
{
	if (!RangeComponent || bIsGhost) return;

	// 1. Clear current list
	OverlappingTargets.Empty();

	// 2. Get all currently overlapping actors
	TArray<AActor*> CurrentlyOverlapping;
	RangeComponent->GetOverlappingActors(CurrentlyOverlapping, TargetClassFilter);

	// 3. Filter results (excluding self and ensuring validity)
	for (AActor* Actor : CurrentlyOverlapping)
	{
		if (Actor && Actor != this)
		{
			OverlappingTargets.AddUnique(Actor);
		}
	}

	// 4. Update targeting state now that the list is refreshed
	SelectTarget();
}
