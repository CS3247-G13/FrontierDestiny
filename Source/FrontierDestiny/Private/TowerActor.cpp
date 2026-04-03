// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerActor.h"
#include "CustomChannels.h"

#include "GlobalTowerSettings.h"
#include "TowerManagerSubsystem.h"
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
	RangeComponent->SetGenerateOverlapEvents(false);
}

void ATowerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

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
	}
}

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
