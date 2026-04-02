// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerActor.h"
#include "CustomChannels.h"

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

	RangeComponent->SetSphereRadius(TowerData.Range);

	RangeComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RangeComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(CC_Enemy, ECR_Overlap);
	RangeComponent->SetGenerateOverlapEvents(false); // enabled only when tower activates
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

	RangeComponent->SetGenerateOverlapEvents(true);
	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &ATowerActor::OnRangeBeginOverlap);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &ATowerActor::OnRangeEndOverlap);

	// Fire BeginOverlap for any enemies already inside the radius
	RangeComponent->UpdateOverlaps();

	if (UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>())
	{
		EnemyManager->OnEnemyDeath.AddUObject(this, &ATowerActor::OnTrackedEnemyDied);
	}

	GetWorldTimerManager().SetTimer(
		RangeCheckTimerHandle,
		this,
		&ATowerActor::CheckEnemiesInRange,
		1.0f,
		true
	);

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
		GetWorldTimerManager().ClearTimer(BuildTimerHandle);

		GetWorldTimerManager().ClearTimer(RangeCheckTimerHandle);

		if (UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>())
		{
			EnemyManager->OnEnemyDeath.RemoveAll(this);
		}
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

void ATowerActor::OnRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

void ATowerActor::OnRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager) return;

	UInstancedStaticMeshComponent* ISMC = Cast<UInstancedStaticMeshComponent>(OtherComp);
	if (!ISMC) return;

	FMassEntityHandle Handle = EnemyManager->GetEnemyEntityHandle(ISMC, OtherBodyIndex);
	if (!Handle.IsSet()) return;
	RemoveTarget(Handle);
}

void ATowerActor::OnTrackedEnemyDied(FMassEntityHandle Handle)
{
	RemoveTarget(Handle);
}

void ATowerActor::AddTarget(FMassEnemyTarget Target)
{
	if (OverlappingTargets.Contains(Target.EntityHandle)) return;

	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	if (!EnemyManager || EnemyManager->CheckEnemyStealth(Target)) return;

	OverlappingTargets.Add(Target.EntityHandle);
	OnTargetEnterRange(Target);
}

void ATowerActor::RemoveTarget(FMassEntityHandle Handle)
{
	if (!OverlappingTargets.Contains(Handle)) return;

	OverlappingTargets.Remove(Handle);

	FMassEnemyTarget Target;
	Target.EntityHandle = Handle;
	OnTargetLeaveRange(Target);
}

void ATowerActor::CheckEnemiesInRange()
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

void ATowerActor::OnTargetEnterRange_Implementation(FMassEnemyTarget Target)
{ }

void ATowerActor::OnTargetLeaveRange_Implementation(FMassEnemyTarget Target)
{ }