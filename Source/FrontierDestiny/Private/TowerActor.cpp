// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerActor.h"

#include "GlobalTowerSettings.h"
#include "TowerManagerSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"

#include "TowerData.h"
#include "GridActor.h"

ATowerActor::ATowerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetupAttachment(RootComponent);

	// Default target filter to any actor
	TargetClassFilter = AActor::StaticClass();

	// Initial detection settings in constructor
	RangeComponent->SetSphereRadius(CachedTowerData.Stats.GetRange());

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
		RangeComponent->SetSphereRadius(CachedTowerData.Stats.GetRange());
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

	TArray<UPrimitiveComponent*> Components;
	GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* PrimComp : Components)
	{
		if (PrimComp)
		{
			if (PrimComp == RangeComponent)
			{
				continue;
			}
			PrimComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		}
	}
}

void ATowerActor::ActivateTower()
{
	bTowerIsInactive = false;
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

void ATowerActor::SetValidOverlayMaterial()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();

	if (!Settings) return;

	TArray<UMeshComponent*> Components;
	GetComponents<UMeshComponent>(Components);

	UMaterialInterface* ValidGhostOverlayMaterial = Settings->GhostMaterialValid.LoadSynchronous();

	for (UMeshComponent* MeshComp : Components)
	{
		if (ValidGhostOverlayMaterial)
		{
			MeshComp->SetOverlayMaterial(ValidGhostOverlayMaterial);
		}
	}
}

void ATowerActor::SetInvalidOverlayMaterial()
{
	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();

	if (!Settings) return;

	TArray<UMeshComponent*> Components;
	GetComponents<UMeshComponent>(Components);

	UMaterialInterface* InvalidGhostOverlayMaterial = Settings->GhostMaterialInvalid.LoadSynchronous();

	for (UMeshComponent* MeshComp : Components)
	{
		if (InvalidGhostOverlayMaterial)
		{
			MeshComp->SetOverlayMaterial(InvalidGhostOverlayMaterial);
		}
	}
}

void ATowerActor::ClearOverlayMaterial()
{
	TArray<UMeshComponent*> Components;
	GetComponents<UMeshComponent>(Components);
	
	for (UMeshComponent* MeshComp : Components)
	{
		MeshComp->SetOverlayMaterial(nullptr);
	}
}

void ATowerActor::BeginPlay()
{
	Super::BeginPlay();

	UTowerManagerSubsystem* TowerManager = GetWorld()->GetGameInstance()->GetSubsystem<UTowerManagerSubsystem>();
	TowerManager->OnTowerUpgraded.AddDynamic(this, &ATowerActor::UpdateStats);

	UpdateStats(TowerID);

	if (bIsGhost)
	{
		InitializeGhostTower();
	}
	else
	{
		bTowerIsInactive = true;
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

	if (UTowerManagerSubsystem* TowerManager = GetWorld()->GetGameInstance()->GetSubsystem<UTowerManagerSubsystem>())
	{
		TowerManager->OnTowerUpgraded.RemoveDynamic(this, &ATowerActor::UpdateStats);
	}
}

// When the tower is upgraded, this is called to sync the tower. This is
// overridable so that children can also update any special stats that they have.
void ATowerActor::UpdateStats(FName ID)
{
	if (ID != TowerID)
	{
		return;
	}

	UTowerManagerSubsystem* TowerManager = GetWorld()->GetGameInstance()->GetSubsystem<UTowerManagerSubsystem>();
	TowerManager->GetTowerBaseStats(TowerID, CachedTowerData);

	if (RangeComponent)
	{
		RangeComponent->SetSphereRadius(CachedTowerData.Stats.GetRange());
	}
}

int32 ATowerActor::GetDamage()
{
	FTowerStats& Stats = CachedTowerData.Stats;
	return Stats.GetDamage();
}

float ATowerActor::GetCooldown()
{
	FTowerStats& Stats = CachedTowerData.Stats;
	return Stats.GetCooldown();
}

int32 ATowerActor::GetHealth()
{
	FTowerStats& Stats = CachedTowerData.Stats;
	return Stats.GetHealth();
}

int32 ATowerActor::GetRange()
{
	FTowerStats& Stats = CachedTowerData.Stats;
	return Stats.GetRange();
}

void ATowerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsGhost || bTowerIsInactive)
	{
		return;
	}

	OnTowerTick(DeltaSeconds);
}

void ATowerActor::DestroyTower()
{
	bTowerIsInactive = true;
	// TODO: Do something to schedule a delete
	GridActor->RemoveTower(
		CornerGridIndex,
		GetActorRotation(),
		this
	);
	Destroy();
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

	if (bIsValidGhost)
	{
		SetValidOverlayMaterial();
	}
	else
	{
		SetInvalidOverlayMaterial();
	}
}

void ATowerActor::SelectTarget()
{
	// Default implementation: no target selection logic
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
