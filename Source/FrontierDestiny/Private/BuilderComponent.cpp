// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"

#include "BuilderComponent.h"

#include "TowerActor.h"
#include "TowerData.h"
#include "GridActor.h"

UBuilderComponent::UBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UBuilderComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuilderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsBuildingModeActive && bIsRaycastBuilder && GhostTowerActor && SelectedTowerData)
	{
		FHitResult Hit;
		if (!TryPerformRaycast(Hit))
		{
			// There is no camera to perform the raycast from, this component cannot be a raycast builder
			UE_LOG(LogTemp, Error, 
				TEXT("BuilderComponent on %s failed to perform raycast. Did you forget to attach a camera?"), *GetName());
			return;
		}

		if (!Hit.bBlockingHit)
		{
			// No hit detected
			return;
		}

		// Check if the raycast collided with the grid
		AGridActor* HitGridActor = Cast<AGridActor>(Hit.GetActor());
		if (!HitGridActor)
		{
			return;
		}

		SetGrid(HitGridActor);

		GridActor->GetSnappedGridIndex(Hit.Location, CurrentGridLocationIndex);
		UpdateGhostStructureRotation();
		UpdateGhostStructureLocation();

		bCanPlaceTower = CheckTowerCanBePlaced();
		UpdateGhostStructureValid();
	}
}

void UBuilderComponent::ActivateBuildingMode()
{
	bIsBuildingModeActive = true;

	if (GhostTowerActor)
	{
		GhostTowerActor->SetActorHiddenInGame(false);
	}
}

void UBuilderComponent::DeactivateBuildingMode()
{
	bIsBuildingModeActive = false;

	if (GhostTowerActor)
	{
		GhostTowerActor->SetActorHiddenInGame(true);
	}
}

void UBuilderComponent::ToggleBuildingMode()
{
	if (bIsBuildingModeActive)
	{
		DeactivateBuildingMode();
	}
	else
	{
		ActivateBuildingMode();
	}
}

void UBuilderComponent::ChangeTowerSelection(UTowerData* NewTowerData)
{
	SelectedTowerData = NewTowerData;
	UpdateGhostStructureBlueprint();

	bCanPlaceTower = CheckTowerCanBePlaced();
	UpdateGhostStructureValid();

	UpdateGhostStructureLocation();
}

bool UBuilderComponent::TryBuildTower()
{
	if (!bCanPlaceTower || !SelectedTowerData)
	{
		return false;
	}

	FTransform SpawnTransform(GhostTowerActor->GetActorRotation(), GhostTowerActor->GetActorLocation());

	ATowerActor* NewTower = GetWorld()->SpawnActorDeferred<ATowerActor>(
		SelectedTowerData->TowerBlueprint,
		SpawnTransform,
		GetOwner(),
		Cast<APawn>(GetOwner()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (NewTower)
	{
		NewTower->bIsGhost = false;
		NewTower->TowerInfo = SelectedTowerData;
		NewTower->FinishSpawning(SpawnTransform);
	}

	if (IsValid(GridActor))
	{
		GridActor->PlaceTower(CurrentGridLocationIndex, GetBuildingRotator(), NewTower);
	}
	return true;
}

void UBuilderComponent::UpdateGhostStructureBlueprint()
{
	if (IsValid(GhostTowerActor))
	{
		GhostTowerActor->Destroy();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	GhostTowerActor = GetWorld()->SpawnActor<ATowerActor>(SelectedTowerData->TowerBlueprint);
	GhostTowerActor->bIsGhost = true;
}

void UBuilderComponent::UpdateGhostStructureValid()
{
	GhostTowerActor->SetGhostValidity(bCanPlaceTower);
}

bool UBuilderComponent::CheckTowerCanBePlaced()
{
	if (!IsValid(GridActor))
	{
		return false;
	}

	if (!GridActor->CanPlaceTower(CurrentGridLocationIndex, GetBuildingRotator(), SelectedTowerData))
	{
		return false;
	}

	return true;

	// TODO: Set to false in the case of overlap with a custom channel Tower Blockers (exclude projectiles)
}

void UBuilderComponent::UpdateGhostStructureLocation()
{
	// We get the correct corner index by making an int vector from the pivot point to the bottom left corner
	// then rotating this by the building rotation, and adding that to the pivot point index
	FVector PivotPointToCornerIndexVector = -FVector(SelectedTowerData->PivotPoint);
	PivotPointToCornerIndexVector = GetBuildingRotator().RotateVector(PivotPointToCornerIndexVector);
	FIntPoint CornerIndex = CurrentGridLocationIndex + FIntPoint(
		FMath::RoundToInt(PivotPointToCornerIndexVector.X),
		FMath::RoundToInt(PivotPointToCornerIndexVector.Y)
	);

	FVector CornerLocation;

	if (IsValid(GridActor))
	{
		GridActor->GetWorldLocationFromGridIndex(CornerIndex, GetBuildingRotator(), CornerLocation);
		GhostTowerActor->SetActorLocation(CornerLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UBuilderComponent::UpdateGhostStructureRotation()
{
	// Update the ghost tower rotation based on player location
	UCameraComponent* Camera = Cast<APawn>(GetOwner())->FindComponentByClass<UCameraComponent>();
	if (!Camera) return;

	float CameraYawRotation = Camera->GetComponentRotation().Yaw;

	// Snap to 90 degree increments
	uint8 Increments = FMath::RoundToInt(CameraYawRotation / 90.f) % 4;
	BuildingRotationRelativeToBuilder = static_cast<ERotation>(Increments);
	GhostTowerActor->SetActorRotation(GetBuildingRotator());
}

void UBuilderComponent::SetGrid(AGridActor* NewGrid)
{
	GridActor = NewGrid;
}

bool UBuilderComponent::TryPerformRaycast(FHitResult& Hit)
{
	if (!IsValid(Cast<APawn>(GetOwner()))) return false;
	UCameraComponent* Camera = Cast<APawn>(GetOwner())->FindComponentByClass<UCameraComponent>();
	if (!Camera) return false;

	FVector Start = Camera->GetComponentLocation();
	FVector ForwardVector = Camera->GetForwardVector();
	float TraceDistance = buildRange;
	FVector End = Start + (ForwardVector * TraceDistance);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Cast<APawn>(GetOwner()));

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_GameTraceChannel1,
		TraceParams
	);

	return bHit;
}

void UBuilderComponent::RotateTower(bool Clockwise)
{
	uint8 Direction = Clockwise ? 1 : 3;
	AddedBuildingRotation = static_cast<ERotation>(
		// Add 4 to avoid negative numbers for our unsigned int
		(static_cast<uint8>(AddedBuildingRotation) + Direction) % 4
		);
}

