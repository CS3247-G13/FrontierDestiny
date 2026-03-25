// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerActor.h"

#include "GlobalTowerSettings.h"
#include "TowerManagerSubsystem.h"
#include "EnemyManagerSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"

#include "TowerData.h"
#include "GridActor.h"

ATowerActor::ATowerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetupAttachment(RootComponent);

	// Initial detection settings in constructor
	RangeComponent->SetSphereRadius(TowerData.Range);

	// Range component is visual only — range detection uses periodic Mass entity queries
	RangeComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	RangeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeComponent->SetGenerateOverlapEvents(false);

	
	OverlapCheckInterval = 0.2f;
}

void ATowerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Update sphere radius in the editor when TowerRange is changed
	if (RangeComponent)
	{
		RangeComponent->SetSphereRadius(TowerData.Range);
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
		0.1f,
		false
	);
}

void ATowerActor::ActivateTower()
{
	bTowerIsInactive = false;

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

	UpdateStats();

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
}

// When the tower is upgraded, this is called to sync the tower. This is
// overridable so that children can also update any special stats that they have.
void ATowerActor::UpdateStats()
{
	UTowerManagerSubsystem* TowerManager = GetWorld()->GetGameInstance()->GetSubsystem<UTowerManagerSubsystem>();
	TowerManager->GetTowerData(TowerID, TowerData);

	if (RangeComponent)
	{
		RangeComponent->SetSphereRadius(TowerData.Range);
	}
}

float ATowerActor::GetStats(FGameplayTag Tag)
{
	const FTowerEffect* Effect = TowerData.Effects.Find(Tag);
	if (!Effect)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetStats: Tag '%s' not found on tower '%s'. Returning 0."), *Tag.ToString(), *GetName());
		return 0.f;
	}
	return Effect->Amount;
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
	FTowerPlacementIntent Placement;
	Placement.PivotPoint = CornerGridIndex;
	Placement.Rotation = GridRelativeRotation;
	Placement.TowerData = TowerData;
	GridActor->RemoveTower(Placement);
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

void ATowerActor::CheckAllOverlaps()
{
	if (bIsGhost) return;

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return;

	TArray<FMassEntityHandle> CurrentlyInRange;
	EnemyManager->GetEntitiesInRange(GetActorLocation(), TowerData.Range, CurrentlyInRange);

	// Entities that just entered range
	for (const FMassEntityHandle& Handle : CurrentlyInRange)
	{
		if (!OverlappingTargets.Contains(Handle))
		{
			OverlappingTargets.Add(Handle);
			FMassEnemyTarget Target;
			Target.EntityHandle = Handle;
			Target.Position = EnemyManager->GetEntityPosition(Handle);
			OnTargetEnterRange(Target);
		}
	}

	// Entities that left range or were destroyed
	TSet<FMassEntityHandle> ToRemove;
	for (const FMassEntityHandle& Handle : OverlappingTargets)
	{
		if (!CurrentlyInRange.Contains(Handle))
		{
			ToRemove.Add(Handle);
		}
	}
	for (const FMassEntityHandle& Handle : ToRemove)
	{
		OverlappingTargets.Remove(Handle);
		FMassEnemyTarget Target;
		Target.EntityHandle = Handle;
		OnTargetLeaveRange(Target);
	}
}

void ATowerActor::OnTargetEnterRange_Implementation(FMassEnemyTarget Target)
{ }

void ATowerActor::OnTargetLeaveRange_Implementation(FMassEnemyTarget Target)
{ }