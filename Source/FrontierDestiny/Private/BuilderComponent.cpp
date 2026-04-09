// Fill out your copyright notice in the Description page of Project Settings.

#include "BuilderComponent.h"
#include "CustomChannels.h"

#include "Kismet/GameplayStatics.h"
#include "TowerManagerSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"

#include "EconomySubsystem.h"
#include "TowerActor.h"
#include "TowerData.h"
#include "GridActor.h"

#include "QuestSubsystem.h"

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
	InitializeGhostPool();
}

void UBuilderComponent::InitializeGhostPool()
{
	UTowerManagerSubsystem* TowerManager = GetWorld()->GetGameInstance()->GetSubsystem<UTowerManagerSubsystem>();
	FTransform Transform;
	Transform.SetLocation(FVector(0.f, 0.f, -100000.f));

	for (const auto& Pair : TowerManager->TowerDataMap)
	{
		FGhostTowerPool Pool;
		UClass* Class = Pair.Value.Class.LoadSynchronous();
		for (int i = 0; i < MaxTowers; i++)
		{
			ATowerActor* Ghost = GetWorld()->SpawnActor<ATowerActor>(Class, Transform);
			Pool.Actors.Add(Ghost);
		}
		GhostTowerPool.Add(Pair.Key, Pool);
	}
}

void UBuilderComponent::InitializeReferences()
{
	EconomyComponent = GetWorld()->GetGameInstance()->GetSubsystem<UEconomySubsystem>();
	QuestComponent = GetWorld()->GetGameInstance()->GetSubsystem<UQuestSubsystem>();
	if (!IsValid(EconomyComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("BuilderComponent on %s could not find EconomySubsystem."), *GetName());
	}
}

void UBuilderComponent::SetupInput(UInputComponent* InputComponent)
{
	Super::SetupInput(InputComponent);

	// Cast the internal InputComponent to the Enhanced version
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		TObjectPtr<UInputAction>* SelectActions[] = {
			&SelectTowerAction1, &SelectTowerAction2, &SelectTowerAction3,
			&SelectTowerAction4, &SelectTowerAction5, &SelectTowerAction6
		};
		for (int32 i = 0; i < 6; i++)
		{
			if (*SelectActions[i])
			{
				EnhancedInputComponent->BindAction(*SelectActions[i], ETriggerEvent::Started,
					this, &UBuilderComponent::OnSelectTowerAction, i + 1);
			}
		}
		
		if (DeselectTowerAction)
		{
			EnhancedInputComponent->BindAction(
				DeselectTowerAction,
				ETriggerEvent::Started,
				this,
				&UBuilderComponent::OnDeselectTowerAction
			);
		}

		if (BuildTowerAction)
		{
			EnhancedInputComponent->BindAction(
				BuildTowerAction,
				ETriggerEvent::Started,
				this,
				&UBuilderComponent::OnBuildTowerActionStart
			);

			EnhancedInputComponent->BindAction(
				BuildTowerAction,
				ETriggerEvent::Completed,
				this,
				&UBuilderComponent::OnBuildTowerActionEnd
			);

			EnhancedInputComponent->BindAction(
				BuildTowerAction,
				ETriggerEvent::Canceled,
				this,
				&UBuilderComponent::OnBuildTowerActionEnd
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
	if (!bIsRaycastBuilder)
	{
		return;
	}

	if (IsValid(ClosestGridActor))
	{
		// ClosestGridActor->LogGridState();
	}

	FHitResult Hit;
	if (!SelectedTower.IsSet())
	{
		// No select tower, means it is in delete mode
		ATowerActor* HitTowerActor;
		if (!TryRaycastToTower(Hit, HitTowerActor))
		{
			UpdateHoveredTower(nullptr);
			return;
		}

		UpdateHoveredTower(HitTowerActor);
		return;
	}

	else
	{
		UpdateHoveredTower(nullptr);

		// Is in build mode
		if (TryRaycastToGrid(Hit))
		{
			if (!bIsLocked)
			{
				ClosestGridActor->GetSnappedGridIndex(Hit.Location, LockedGridLocationStart);
			}
			ClosestGridActor->GetSnappedGridIndex(Hit.Location, LockedGridLocationEnd);
			
			UpdateGhostStructure();
		}

		// Whether you hit a grid or not, we still update rotation and can place
		if (!bIsLocked)
		{
			UpdateGhostStructureRotation();
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

	UpdatePostProcessComponentProgress();
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
	UpdatePostProcessComponentProgress();
}

void UBuilderComponent::CheckForClosestGridActor()
{
	// TODO: Make this get from some kind of grid manager
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridActor::StaticClass(), Actors);

	AActor* ClosestActor = nullptr;
	float Distance = MAX_FLT;
	for (AActor*& Actor : Actors)
	{
		float CurrentDistance = (Actor->GetTransform().GetLocation() - Pawn->GetTransform().GetLocation()).Length();
		if (CurrentDistance < Distance)
		{
			ClosestActor = Actor;
			Distance = CurrentDistance;
		}
	}
	if (!IsValid(ClosestActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("No grid actors found!"));
		return;
	}

	AGridActor* CastedGridActor = Cast<AGridActor>(ClosestActor);
	if (!IsValid(CastedGridActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Somehow the casting to AGridActor failed!"));
		return;
	}
	if (CastedGridActor != ClosestGridActor)
	{
		ClosestGridActor = CastedGridActor;
		// There is a new closest grid actor! We will need to update the material
		UpdatePostProcessComponentOffset();
		ClosestGridActor->UpdateOccupancyTexture(TArray<FTowerPlacementIntent>());
		UpdatePostProcessComponentOccupancyBitmask();
	}
}

void UBuilderComponent::UpdatePostProcessComponentProgress()
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

void UBuilderComponent::UpdatePostProcessComponentOffset()
{
	if (IsValid(ClosestGridActor))
	{
		UE_LOG(LogTemp, Display, TEXT("Closest Grid Actor: %s"), *ClosestGridActor->GetName());
		GridVisualMID->SetVectorParameterValue(TEXT("Grid Offset"), ClosestGridActor->GetTransform().GetLocation());
		GridVisualMID->SetVectorParameterValue(TEXT("Grid Size"), FVector(ClosestGridActor->ScaleX, ClosestGridActor->ScaleY, 0.f));
		GridVisualMID->SetScalarParameterValue(TEXT("Grid Rotation"), ClosestGridActor->GetActorRotation().Yaw);
		GridVisualMID->SetScalarParameterValue(TEXT("Grid Size"), ClosestGridActor->CellSize);
	}
}

void UBuilderComponent::UpdatePostProcessComponentOccupancyBitmask()
{
	if (!IsValid(GridVisualMID))
	{
		UE_LOG(LogTemp, Warning, TEXT("No Grid Texture Instance found. Something went wrong!"));
		return;
	}

	if (!IsValid(ClosestGridActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Grid actor not found or cast failed."));
		return;
	}

	GridVisualMID->SetVectorParameterValue("Occupancy Bitmask Size", ClosestGridActor->OccupancyTextureSize);
	GridVisualMID->SetTextureParameterValue("Occupancy Bitmask", ClosestGridActor->OccupancyTexture);
}

// MODE ACTIVATION
void UBuilderComponent::ActivateMode()
{
	Super::ActivateMode();
	CheckForClosestGridActor();
	EnterGridVisual();

	QuestComponent->StartQuest("Onboard_5");

	OnDeselectTowerAction(FInputActionValue());
}

void UBuilderComponent::DeactivateMode()
{
	Super::DeactivateMode();
	ExitGridVisual();
	
	ChangeTowerSelection(TOptional<FName>());
	UpdateHoveredTower(nullptr);
	UpdateGhostStructure();

	UE_LOG(LogTemp, Display, TEXT("Exit"));
}

// INPUT ACTIONS
void UBuilderComponent::OnBuildTowerActionStart(const FInputActionValue& Value)
{
	if (SelectedTower.IsSet())
	{
		bIsLocked = true;
	}
	else if (IsValid(HoveredTower))
	{
		DeleteHoveredTower();
	}
}

void UBuilderComponent::OnBuildTowerActionEnd(const FInputActionValue& Value)
{
	TryBuildTowers();
	QuestComponent->StartQuest("Onboard_7");
	bIsLocked = false;
}


void UBuilderComponent::OnSelectTowerAction(int32 KeyNumber)
{
	UTowerManagerSubsystem* TowerManager;

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!GI) return;

	TowerManager = GI->GetSubsystem<UTowerManagerSubsystem>();
	if (!TowerManager) return;

	{
		SelectedPath.Add(KeyNumber);
		
		// Dear tower manager, is there anything there?
		if (!TowerManager->CheckPathUnlocked(SelectedPath))
		{
			SelectedPath.RemoveAt(SelectedPath.Num() - 1);
			OnUnassignedNumberPressed.Broadcast(KeyNumber);
			return;
		}
		
		OnTowerSelectionChange.Broadcast();
		QuestComponent->StartQuest("Onboard_6");
		ChangeTowerSelection(TOptional<FName>(TowerManager->GetPathTower(SelectedPath).ID));
	}
}

void UBuilderComponent::OnDeselectTowerAction(const FInputActionValue& Value)
{
	OnTowerBuildingNotification.Broadcast("");
	SelectedPath.Empty();
	ChangeTowerSelection(TOptional<FName>());
	OnTowerSelectionChange.Broadcast();
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
		UpdateGhostStructure();
		if (IsValid(ClosestGridActor))
		{
			ClosestGridActor->UpdateOccupancyTexture(TArray<FTowerPlacementIntent>());
			UpdatePostProcessComponentOccupancyBitmask();
		}
		OnTowerDeselected.Broadcast();
		return;
	}

	OnTowerSelected.Broadcast();
}

void UBuilderComponent::TryBuildTowers()
{
	if (!SelectedTower.IsSet() || !IsValid(ClosestGridActor))
	{
		return;
	}

	FTowerData SelectedTowerData;
	GetSelectedTowerData(SelectedTowerData);

	// Get the locations to place the towers
	FTowerPlacementIntent StartPlacement;
	StartPlacement.PivotPoint = LockedGridLocationStart;
	StartPlacement.Rotation = GetBuildingRotator();
	StartPlacement.TowerData = SelectedTowerData;

	FTowerPlacementIntent EndPlacement(StartPlacement);
	EndPlacement.PivotPoint = LockedGridLocationEnd;

	TArray<FTowerPlacementIntent> Placements;
	ClosestGridActor->GetTowerPlacementsInLine(StartPlacement, EndPlacement, MaxTowers, Placements);

	bool bAnyBuilt = false;
	for (const FTowerPlacementIntent& Placement : Placements)
	{
		if (!ClosestGridActor->CanPlaceTower(Placement))
		{
			continue;
		}
		if (IsValid(EconomyComponent))
		{
			if (EconomyComponent->TryDeductFunds(SelectedTowerData.Cost) == false)
			{
				break;
			}
		}
		FTransform Transform;
		ClosestGridActor->GetTowerPlacementLocationFromGridIndex(Placement, Transform);

		ATowerActor* NewTower = GetWorld()->SpawnActorDeferred<ATowerActor>(
			SelectedTowerData.Class.LoadSynchronous(),
			Transform,
			GetOwner(),
			Cast<APawn>(GetOwner()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (NewTower)
		{
			NewTower->bIsGhost = false;
			NewTower->FinishSpawning(Transform);
			NewTower->GridActor = ClosestGridActor;
			NewTower->CornerGridIndex = Placement.PivotPoint;
			NewTower->GridRelativeRotation = Placement.Rotation;
			bAnyBuilt = true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn NewTower on %s"), *GetName());
		}

		if (IsValid(ClosestGridActor))
		{
			ClosestGridActor->PlaceTower(Placement);
			ClosestGridActor->UpdateOccupancyTexture(TArray<FTowerPlacementIntent>());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get GridActor to spawn NewTower on %s"), *GetName());
		}
	}

	if (bAnyBuilt)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), BuildSound);
	}
	else
	{
		UGameplayStatics::PlaySound2D(GetWorld(), FailBuildSound);
	}

}

void UBuilderComponent::UpdateGhostStructure()
{
	for (ATowerActor* GhostTower : ActiveGhostTowers)
	{
		GhostTower->SetActorLocation(FVector(0.f, 0.f, -100000.f));
	}

	if (!IsValid(ClosestGridActor))
	{
		return;
	}
	if (!SelectedTower.IsSet())
	{
		return;
	}

	FTowerData SelectedTowerData;
	GetSelectedTowerData(SelectedTowerData);


	// Get the locations to place the towers
	FTowerPlacementIntent StartPlacement;
	StartPlacement.PivotPoint = LockedGridLocationStart;
	StartPlacement.Rotation = GetBuildingRotator();
	StartPlacement.TowerData = SelectedTowerData;

	FTowerPlacementIntent EndPlacement(StartPlacement);
	EndPlacement.PivotPoint = LockedGridLocationEnd;

	TArray<FTowerPlacementIntent> Placements;
	ClosestGridActor->GetTowerPlacementsInLine(StartPlacement, EndPlacement, 10, Placements);

	ActiveGhostTowers.Empty();

	int32 Index = 0;
	for (const FTowerPlacementIntent& Placement : Placements)
	{
		DisplayGhostTower(Placement.PivotPoint, Placement.TowerData, Index);
		Index++;
	}

	ClosestGridActor->UpdateOccupancyTexture(Placements);
	UpdatePostProcessComponentOccupancyBitmask();
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

	if (IsValid(ClosestGridActor))
	{
		LookAtYawRotation -= ClosestGridActor->GetActorRotation().Yaw;
		LookAtYawRotation = FRotator::ClampAxis(LookAtYawRotation);
	}

	// Snap to 90 degree increments
	uint8 Increments = FMath::RoundToInt(LookAtYawRotation / 90.f) % 4;
	BuildingRotationRelativeToBuilder = static_cast<ERotation>(Increments);
	UpdateGhostStructure();
	// GhostTowerActor->SetActorRotation(GetBuildingRotator());
}

bool UBuilderComponent::TryRaycastToGrid(FHitResult& Hit)
{
	if (!IsValid(Camera)) return false;

	FVector Start = Camera->GetComponentLocation();
	FVector ForwardVector = Camera->GetForwardVector();
	float TraceDistance = GetPlayerData().BuildRange;
	FVector End = Start + (ForwardVector * TraceDistance);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Cast<APawn>(GetOwner()));
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		CC_Grid,
		TraceParams
	);

	if (!IsValid(ClosestGridActor))
	{
		return false;
	}

	return bHit;
}

bool UBuilderComponent::TryRaycastToTower(FHitResult& Hit, ATowerActor*& HitTowerActor)
{
	if (!IsValid(Camera)) return false;

	FVector Start = Camera->GetComponentLocation();
	FVector ForwardVector = Camera->GetForwardVector();
	float TraceDistance = GetPlayerData().BuildRange;
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

	HitTowerActor = Cast<ATowerActor>(Hit.GetActor());
	if (!IsValid(HitTowerActor))
	{
		return false;
	}

	return bHit;
}

void UBuilderComponent::DisplayGhostTower(const FIntPoint& PivotPointIndex, const FTowerData& TowerData, int GhostTowerIndex)
{
	ATowerActor* GhostTowerActor;

	FGhostTowerPool* Pool = GhostTowerPool.Find(TowerData.ID);
	if (Pool->Actors.Num() <= GhostTowerIndex)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());
		UClass* Class = TowerData.Class.LoadSynchronous();
		for (int i = 0; i < GhostTowerIndex - Pool->Actors.Num() + 1; i++)
		{
			GhostTowerActor = GetWorld()->SpawnActor<ATowerActor>(Class);
			GhostTowerActor->bIsGhost = true;
		}
	}

	GhostTowerActor = Pool->Actors[GhostTowerIndex];

	FTransform Transform;
	if (IsValid(ClosestGridActor))
	{
		FTowerPlacementIntent Placement;
		Placement.PivotPoint = PivotPointIndex;
		Placement.Rotation = GetBuildingRotator();
		Placement.TowerData = TowerData;
		ClosestGridActor->GetTowerPlacementLocationFromGridIndex(Placement, Transform);
		GhostTowerActor->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);

		bool bCanPlaceTower = false;
		if (UGameInstance* GI = GetWorld()->GetGameInstance())
		{
			if (UEconomySubsystem* Economy = GI->GetSubsystem<UEconomySubsystem>())
			{
				bCanPlaceTower = Economy->HasSufficientFunds(TowerData.Cost);
			}
		}

		if (!bCanPlaceTower)
		{
			OnTowerBuildingNotification.Broadcast("Insufficient funds");
		}
		else
		{
			OnTowerBuildingNotification.Broadcast("");
		}

		bCanPlaceTower &= ClosestGridActor->CanPlaceTower(Placement);
		if (!bCanPlaceTower)
		{
			GhostTowerActor->SetInvalidOverlayMaterial();
		}
		else
		{
			GhostTowerActor->SetValidOverlayMaterial();
		}
		ActiveGhostTowers.Add(GhostTowerActor);
	}
}

void UBuilderComponent::RotateTower(bool Clockwise)
{
	uint8 Direction = Clockwise ? 1 : 3;
	AddedBuildingRotation = static_cast<ERotation>(
		// Add 4 to avoid negative numbers for our unsigned int
		(static_cast<uint8>(AddedBuildingRotation) + Direction) % 4
		);
	UpdateGhostStructure();
}

void UBuilderComponent::DeleteHoveredTower()
{
	ATowerActor* Temp = HoveredTower;
	UpdateHoveredTower(nullptr);
	Temp->DestroyTower();
	UGameplayStatics::PlaySound2D(GetWorld(), DestroyTowerSound);
	if (IsValid(ClosestGridActor))
	{
		ClosestGridActor->UpdateOccupancyTexture(TArray<FTowerPlacementIntent>());
	}
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
		OnHoverTowerStop.Broadcast();
	}

	HoveredTower = NewHoveredTower;

	if (IsValid(HoveredTower))
	{
		HoveredTower->SetInvalidOverlayMaterial();
		OnHoverTowerStart.Broadcast();
	}
}

void UBuilderComponent::GetSelectedTowerData(FTowerData& TowerData)
{
	if (!SelectedTower.IsSet())
	{
		return;
	}

	if (SelectedTower.GetValue() == LoadedTower)
	{
		TowerData = CachedTowerData;
	}

	UTowerManagerSubsystem* TowerManager = GetWorld()->GetGameInstance()->GetSubsystem<UTowerManagerSubsystem>();
	TowerManager->GetTowerData(SelectedTower.GetValue(), CachedTowerData);
	LoadedTower = SelectedTower.GetValue();
	TowerData = CachedTowerData;
}
