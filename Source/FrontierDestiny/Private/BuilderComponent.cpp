// Fill out your copyright notice in the Description page of Project Settings.

#include "BuilderComponent.h"

#include "Kismet/GameplayStatics.h"
#include "TowerManagerSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"

#include "EconomyComponent.h"
#include "TowerActor.h"
#include "TowerData.h"
#include "GridActor.h"

UBuilderComponent::UBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// INITIALIZATION
void UBuilderComponent::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeReferences();
	InitializePostProcessMaterial();
}

void UBuilderComponent::InitializeReferences()
{
	EconomyComponent = PlayerController->FindComponentByClass<UEconomyComponent>();
	if (!IsValid(EconomyComponent))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("BuilderComponent on %s could not find an economy component on its player controller owner."), *GetName());
	}
}

void UBuilderComponent::SetupInput(UInputComponent* InputComponent)
{
	Super::SetupInput(InputComponent);

	// Cast the internal InputComponent to the Enhanced version
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (SelectTowerAction)
		{
			EnhancedInputComponent->BindAction(
				SelectTowerAction,
				ETriggerEvent::Started,
				this,
				&UBuilderComponent::OnSelectTowerAction
			);
		}

		if (BuildTowerAction)
		{
			EnhancedInputComponent->BindAction(
				BuildTowerAction,
				ETriggerEvent::Started,
				this,
				&UBuilderComponent::OnBuildTowerAction
			);
		}

		if (RotateTowerAction)
		{
			EnhancedInputComponent->BindAction(
				RotateTowerAction,
				ETriggerEvent::Started,
				this,
				&UBuilderComponent::OnRotateTowerAction
			);
		}
	}
}

// TICK
void UBuilderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UpdateGridVisualState(DeltaTime);
}

void UBuilderComponent::TickWhenActive()
{
	if (bIsRaycastBuilder)
	{
		FHitResult Hit;
		if (!TryPerformRaycast(Hit))
		{
			UpdateHoveredTower(nullptr);
			return;
		}

		// Check if the raycast collided with the grid
		AGridActor* HitGridActor = Cast<AGridActor>(Hit.GetActor());
		if (HitGridActor && SelectedTower.IsSet() && IsValid(GhostTowerActor))
		{
			SetGrid(HitGridActor);

			GridActor->GetSnappedGridIndex(Hit.Location, CurrentGridLocationIndex);

			UpdateGhostStructureRotation();
			UpdateGhostStructureLocation();

			bCanPlaceTower = CheckTowerCanBePlaced();
			UpdateGhostStructureValid();
		}

		ATowerActor* HitTowerActor = Cast<ATowerActor>(Hit.GetActor());
		if (HitTowerActor && !SelectedTower.IsSet())
		{
			UpdateHoveredTower(HitTowerActor);
		}
		else
		{
			UpdateHoveredTower(nullptr);
		}

	}
}

// GRID VISUALS
void UBuilderComponent::EnterGridVisual()
{
	GridVisualState = EGridVisualState::FadingIn;
}

void UBuilderComponent::ExitGridVisual()
{
	GridVisualState = EGridVisualState::FadingOut;
}

void UBuilderComponent::UpdateGridVisualState(float DeltaSeconds)
{
	switch (GridVisualState)
	{
	case EGridVisualState::FadingIn:
		NormalizedGridVisualProgress += DeltaSeconds * FadeInSpeed;
		NormalizedGridVisualProgress = FMath::Clamp(NormalizedGridVisualProgress, 0.f, 1.f);
		break;
	case EGridVisualState::FadingOut:
		NormalizedGridVisualProgress -= DeltaSeconds * FadeOutSpeed;
		NormalizedGridVisualProgress = FMath::Clamp(NormalizedGridVisualProgress, 0.f, 1.f);
		break;
	}

	UpdatePostProcessComponent();
}

void UBuilderComponent::InitializePostProcessMaterial()
{
	// Create the Dynamic Material Instance
	GridVisualMID = UMaterialInstanceDynamic::Create(GridVisualMaterial, this);

	// Create the Post Process Component
	PostProcessComponent = NewObject<UPostProcessComponent>(GetOwner());
	PostProcessComponent->RegisterComponent();

	PostProcessComponent->bUnbound = true;
	PostProcessComponent->Priority = 10.f;

	PostProcessComponent->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, GridVisualMID));
	UpdatePostProcessComponent();
}

void UBuilderComponent::UpdatePostProcessComponent()
{
	if (!IsValid(PostProcessComponent))
	{
		return;
	}

	if (IsValid(FadeCurve))
	{
		GridVisualMID->SetScalarParameterValue(TEXT("Alpha"), FadeCurve->GetFloatValue(NormalizedGridVisualProgress));
	}
	else
	{
		GridVisualMID->SetScalarParameterValue(TEXT("Alpha"), NormalizedGridVisualProgress);
	}

}

// MODE ACTIVATION
void UBuilderComponent::ActivateMode()
{
	Super::ActivateMode();
	EnterGridVisual();
	if (GhostTowerActor)
	{
		GhostTowerActor->SetActorHiddenInGame(false);
	}
}

void UBuilderComponent::DeactivateMode()
{
	Super::DeactivateMode();
	ExitGridVisual();
	if (GhostTowerActor)
	{
		GhostTowerActor->SetActorHiddenInGame(true);
	}
	
	ChangeTowerSelection(TOptional<FName>());
	UpdateHoveredTower(nullptr);

	UE_LOG(LogTemp, Display, TEXT("Exit"));
}

// INPUT ACTIONS
void UBuilderComponent::OnBuildTowerAction(const FInputActionValue& Value)
{
	if (SelectedTower.IsSet())
	{
		TryBuildTower();
	}
	else if (IsValid(HoveredTower))
	{
		DeleteHoveredTower();
	}
}

void UBuilderComponent::OnSelectTowerAction(const FInputActionValue& Value)
{
	int32 KeyNumber = FMath::RoundToInt(Value.Get<float>());
	if (KeyNumber <= AvailableTowers.Num() && KeyNumber > 0)
	{
		if (SelectedTower.IsSet() && AvailableTowers[KeyNumber - 1] == SelectedTower.GetValue())
		{
			// Deselect if the same tower is selected again
			ChangeTowerSelection(TOptional<FName>());
			return;
		}
		ChangeTowerSelection(AvailableTowers[KeyNumber - 1]);
	}
}

void UBuilderComponent::OnRotateTowerAction(const FInputActionValue& Value)
{
	RotateTower(Value.Get<float>() > 0);
}

// TOWER BUILDING
void UBuilderComponent::ChangeTowerSelection(TOptional<FName> NewTower)
{
	SelectedTower = NewTower;

	if (!NewTower.IsSet())
	{
		if (IsValid(GhostTowerActor))
		{
			GhostTowerActor->Destroy();
			GhostTowerActor = nullptr;
		}
		return;
	}

	UpdateGhostStructureBlueprint();

	bCanPlaceTower = CheckTowerCanBePlaced();
	UpdateGhostStructureValid();
	UpdateGhostStructureLocation();
}

bool UBuilderComponent::TryBuildTower()
{
	if (!bCanPlaceTower || !SelectedTower.IsSet())
	{
		return false;
	}

	FTowerData SelectedTowerData;
	GetSelectedTowerData(SelectedTowerData);

	if (IsValid(EconomyComponent))
	{
		if (EconomyComponent->TryDeductFunds(SelectedTowerData.Cost) == false)
		{
			return false;
		}
	}

	FTransform SpawnTransform(GhostTowerActor->GetActorRotation(), GhostTowerActor->GetActorLocation());

	ATowerActor* NewTower = GetWorld()->SpawnActorDeferred<ATowerActor>(
		SelectedTowerData.Class.LoadSynchronous(),
		SpawnTransform,
		GetOwner(),
		Cast<APawn>(GetOwner()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (NewTower)
	{
		NewTower->bIsGhost = false;
		NewTower->FinishSpawning(SpawnTransform);
		NewTower->GridActor = GridActor;
		NewTower->CornerGridIndex = CurrentGridLocationIndex;

		UGameplayStatics::PlaySound2D(GetWorld(), BuildSound);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn NewTower on %s"), *GetName());
	}

	if (IsValid(GridActor))
	{
		GridActor->PlaceTower(CurrentGridLocationIndex, GetBuildingRotator(), NewTower);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get GridActor to spawn NewTower on %s"), *GetName());
	}
	return true;
}

void UBuilderComponent::UpdateGhostStructureBlueprint()
{
	if (IsValid(GhostTowerActor))
	{
		GhostTowerActor->Destroy();
	}

	FTowerData SelectedTowerData;
	GetSelectedTowerData(SelectedTowerData);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	GhostTowerActor = GetWorld()->SpawnActor<ATowerActor>(SelectedTowerData.Class.LoadSynchronous());
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
	if (!SelectedTower.IsSet())
	{
		return false;
	}


	FTowerData SelectedTowerData;
	GetSelectedTowerData(SelectedTowerData);

	if (!GridActor->CanPlaceTower(CurrentGridLocationIndex, GetBuildingRotator(), SelectedTowerData))
	{
		return false;
	}

	if (IsValid(EconomyComponent))
	{
		if (SelectedTowerData.Cost > 0 && !(EconomyComponent->HasSufficientFunds(SelectedTowerData.Cost)))
		{
			return false;
		}
	}
	
	return true;

	// TODO: Set to false in the case of overlap with a custom channel Tower Blockers (exclude projectiles)
}

void UBuilderComponent::UpdateGhostStructureLocation()
{
	FTowerData SelectedTowerData;
	GetSelectedTowerData(SelectedTowerData);

	// We get the correct corner index by making an int vector from the pivot point to the bottom left corner
	// then rotating this by the building rotation, and adding that to the pivot point index
	FVector PivotPointToCornerIndexVector = -FVector(SelectedTowerData.PivotPoint);
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
	float LookAtYawRotation;

	// Update the ghost tower rotation based on player location
	if (IsValid(Camera))
	{
		LookAtYawRotation = Camera->GetComponentRotation().Yaw;
	}
	else
	{
		LookAtYawRotation = GetOwner()->GetActorRotation().Yaw;
	}

	// Snap to 90 degree increments
	uint8 Increments = FMath::RoundToInt(LookAtYawRotation / 90.f) % 4;
	BuildingRotationRelativeToBuilder = static_cast<ERotation>(Increments);
	GhostTowerActor->SetActorRotation(GetBuildingRotator());
}

void UBuilderComponent::SetGrid(AGridActor* NewGrid)
{
	GridActor = NewGrid;
}

bool UBuilderComponent::TryPerformRaycast(FHitResult& Hit)
{
	if (!IsValid(Camera)) return false;

	FVector Start = Camera->GetComponentLocation();
	FVector ForwardVector = Camera->GetForwardVector();
	float TraceDistance = BuildRange;
	FVector End = Start + (ForwardVector * TraceDistance);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Cast<APawn>(GetOwner()));
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		TraceParams
	);

	if (!bHit)
	{
		return false;
	}

	// Delete mode
	if (!SelectedTower.IsSet())
	{
		return true;
	}

	bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Hit.Location + FVector(0.f, 0.f, BuildZCheck),
		Hit.Location + FVector(0.f, 0.f, -BuildZCheck),
		ECC_GameTraceChannel1, // Grid only
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

void UBuilderComponent::DeleteHoveredTower()
{
	HoveredTower->DestroyTower();
	UpdateHoveredTower(nullptr);
}

void UBuilderComponent::UpdateHoveredTower(ATowerActor* NewHoveredTower)
{
	if (HoveredTower == NewHoveredTower)
	{
		return;
	}
	if (IsValid(HoveredTower))
	{
		HoveredTower->ClearOverlayMaterial();
	}
	HoveredTower = NewHoveredTower;
	if (IsValid(HoveredTower))
	{
		HoveredTower->SetInvalidOverlayMaterial();
	}
}

void UBuilderComponent::GetSelectedTowerData(FTowerData& TowerData)
{
	if (!SelectedTower.IsSet())
	{
		return;
	}

	if (SelectedTower.GetValue()  == LoadedTower)
	{
		TowerData = CachedTowerData;
	}

	UTowerManagerSubsystem* TowerManager = GetWorld()->GetGameInstance()->GetSubsystem<UTowerManagerSubsystem>();
	TowerManager->GetTowerBaseStats(SelectedTower.GetValue(), CachedTowerData);
	LoadedTower = SelectedTower.GetValue();
	TowerData = CachedTowerData;
}
