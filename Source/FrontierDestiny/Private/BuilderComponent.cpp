// Fill out your copyright notice in the Description page of Project Settings.

#include "BuilderComponent.h"

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
	EconomyComponent = GetWorld()->GetGameInstance()->GetSubsystem<UEconomySubsystem>();
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
		if (!IsValid(GhostTowerActor))
		{
			// Something went very wrong
			UE_LOG(LogTemp, Warning, TEXT("The ghost actor is not valid despite being in build mode. something is quite wrong."));
			return;
		}

		// Is in build mode
		if (TryRaycastToGrid(Hit))
		{
			ClosestGridActor->GetSnappedGridIndex(Hit.Location, CurrentGridLocationIndex);
			UpdateGhostStructureLocation();
		}

		// Whether you hit a grid or not, we still update rotation and can place
		UpdateGhostStructureRotation();
		bCanPlaceTower = CheckTowerCanBePlaced();
		UpdateGhostStructureValid();
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

	FIntPoint GridSize = ClosestGridActor->GridSize;

	// 1. Calculate Power of 2 dimensions
	int32 TexWidth = FMath::RoundUpToPowerOfTwo(GridSize.X);
	int32 TexHeight = FMath::RoundUpToPowerOfTwo(GridSize.Y);

	GridVisualMID->SetVectorParameterValue("Occupancy Bitmask Size", FVector(TexWidth, TexHeight, 0.f));

	// 2. Create the transient texture
	// Using PF_B8G8R8A8 (Blue, Green, Red, Alpha)
	if (!OccupancyTexture || OccupancyTexture->GetSizeX() != TexWidth || OccupancyTexture->GetSizeY() != TexHeight)
	{
		OccupancyTexture = UTexture2D::CreateTransient(TexWidth, TexHeight, PF_B8G8R8A8);
		if (!OccupancyTexture) return;

		OccupancyTexture->CompressionSettings = TC_VectorDisplacementmap;
		OccupancyTexture->SRGB = false;
		OccupancyTexture->Filter = TF_Nearest;
		OccupancyTexture->UpdateResource();
	}

	// 3. Lock the texture for editing
	FTexture2DMipMap& Mip = OccupancyTexture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	uint8* RawData = (uint8*)Data;

	// Clear buffer to black/transparent
	FMemory::Memzero(RawData, TexWidth * TexHeight * 4);
	
	TArray<int32> CurrentFootprintIndices;
	TArray<int32> CurrentBoundaryIndices;
	if (SelectedTower.IsSet())
	{
		FTowerData SelectedTowerData;
		GetSelectedTowerData(SelectedTowerData);
		ClosestGridActor->GetTowerGridIndices(CurrentGridLocationIndex, GetBuildingRotator(), SelectedTowerData, CurrentFootprintIndices, CurrentBoundaryIndices);
	}

	// 4. Fill the buffer based on your 1D-as-2D arrays
	for (int32 y = 0; y < GridSize.Y; y++)
	{
		for (int32 x = 0; x < GridSize.X; x++)
		{
			int32 GridIndex = y * GridSize.X + x;
			int32 PixelIndex = (y * TexWidth + x) * 4;

			// Logic: OccupiedFootprint = Blue Channel, OccupiedBoundary = Green Channel
			// CurrentFootprint = Red Channel, CurrentBoundary = Alpha Channel
			
			// Using 255 for full intensity
			if (ClosestGridActor->Occupied.IsValidIndex(GridIndex) && ClosestGridActor->Occupied[GridIndex])
			{
				RawData[PixelIndex + 0] = 255; // B (Blue for Footprint)
			}

			if (ClosestGridActor->BoundaryOccupied.IsValidIndex(GridIndex) && ClosestGridActor->BoundaryOccupied[GridIndex] > 0)
			{
				RawData[PixelIndex + 1] = 255; // G (Red for Boundary)
			}
		}
	}

	for (int32 Index : CurrentFootprintIndices)
	{
		if (!ClosestGridActor->Occupied.IsValidIndex(Index))
		{
			continue; // Out of bounds
		}

		// Convert 1D Grid Index back to 2D Grid Coordinates
		int32 x = Index % GridSize.X;
		int32 y = Index / GridSize.X;

		// Convert 2D Grid Coordinates to Texture Pixel Index
		int32 PixelIndex = (y * TexWidth + x) * 4;

		RawData[PixelIndex + 2] = 255; // R channel
	}

	// 6. Overlay Ghost Boundary (Alpha Channel)
	for (int32 Index : CurrentBoundaryIndices)
	{
		if (!ClosestGridActor->BoundaryOccupied.IsValidIndex(Index))
		{
			continue; // Out of bounds
		}

		int32 x = Index % GridSize.X;
		int32 y = Index / GridSize.X;
		int32 PixelIndex = (y * TexWidth + x) * 4;

		RawData[PixelIndex + 3] = 255; // A channel
	}

	Mip.BulkData.Unlock();
	OccupancyTexture->UpdateResource();

	// 5. Pass to Material
	GridVisualMID->SetTextureParameterValue("Occupancy Bitmask", OccupancyTexture);
}

// MODE ACTIVATION
void UBuilderComponent::ActivateMode()
{
	Super::ActivateMode();
	CheckForClosestGridActor();
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
	UpdatePostProcessComponentOccupancyBitmask();
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
			UpdatePostProcessComponentOccupancyBitmask();
			return;
		}
		ChangeTowerSelection(AvailableTowers[KeyNumber - 1]);
		UpdatePostProcessComponentOccupancyBitmask();
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
	UpdatePostProcessComponentOccupancyBitmask();
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
		if (!EconomyComponent->TryDeductFunds(SelectedTowerData.Cost))
		{
			return false;
		}
	}

	ATowerActor* NewTower = GetWorld()->SpawnActorDeferred<ATowerActor>(
		SelectedTowerData.Class.LoadSynchronous(),
		GhostTowerActor->GetTransform(),
		GetOwner(),
		Cast<APawn>(GetOwner()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (NewTower)
	{
		NewTower->bIsGhost = false;
		NewTower->FinishSpawning(GhostTowerActor->GetTransform());
		NewTower->GridActor = ClosestGridActor;
		NewTower->CornerGridIndex = CurrentGridLocationIndex;
		NewTower->GridRelativeRotation = GetBuildingRotator();
		UGameplayStatics::PlaySound2D(GetWorld(), BuildSound);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn NewTower on %s"), *GetName());
	}

	if (IsValid(ClosestGridActor))
	{
		ClosestGridActor->PlaceTower(CurrentGridLocationIndex, GetBuildingRotator(), NewTower);
		UpdatePostProcessComponentOccupancyBitmask();
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
	if (GhostTowerActor->TowerID != SelectedTower.GetValue())
	{
		UE_LOG(LogTemp, Warning, TEXT("The ghost tower's ID %s does not match the selected tower data's ID %s. Something is wrong with the data."), *GhostTowerActor->TowerID.ToString(), *SelectedTower.GetValue().ToString());
	}
}

void UBuilderComponent::UpdateGhostStructureValid()
{
	GhostTowerActor->SetGhostValidity(bCanPlaceTower);
}

bool UBuilderComponent::CheckTowerCanBePlaced()
{
	if (!IsValid(ClosestGridActor))
	{
		return false;
	}
	if (!SelectedTower.IsSet())
	{
		return false;
	}


	FTowerData SelectedTowerData;
	GetSelectedTowerData(SelectedTowerData);

	if (!ClosestGridActor->CanPlaceTower(CurrentGridLocationIndex, GetBuildingRotator(), SelectedTowerData))
	{
		return false;
	}

	if (IsValid(EconomyComponent))
	{
		if (!SelectedTowerData.Cost.IsZero() && !EconomyComponent->HasSufficientFunds(SelectedTowerData.Cost))
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

	FTransform Transform;

	if (IsValid(ClosestGridActor))
	{
		ClosestGridActor->GetTowerPlacementLocationFromGridIndex(CornerIndex, GetBuildingRotator(), SelectedTowerData, Transform);
		GhostTowerActor->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	}
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

	// Snap to 90 degree increments
	uint8 Increments = FMath::RoundToInt(LookAtYawRotation / 90.f) % 4;
	BuildingRotationRelativeToBuilder = static_cast<ERotation>(Increments);
	UpdateGhostStructureLocation();
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
		ECC_GameTraceChannel1,
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


void UBuilderComponent::RotateTower(bool Clockwise)
{
	uint8 Direction = Clockwise ? 1 : 3;
	AddedBuildingRotation = static_cast<ERotation>(
		// Add 4 to avoid negative numbers for our unsigned int
		(static_cast<uint8>(AddedBuildingRotation) + Direction) % 4
		);
	UpdatePostProcessComponentOccupancyBitmask();
}

void UBuilderComponent::DeleteHoveredTower()
{
	HoveredTower->DestroyTower();
	UpdateHoveredTower(nullptr);
	UpdatePostProcessComponentOccupancyBitmask();
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

	if (SelectedTower.GetValue() == LoadedTower)
	{
		TowerData = CachedTowerData;
	}

	UTowerManagerSubsystem* TowerManager = GetWorld()->GetGameInstance()->GetSubsystem<UTowerManagerSubsystem>();
	TowerManager->GetTowerData(SelectedTower.GetValue(), CachedTowerData);
	LoadedTower = SelectedTower.GetValue();
	TowerData = CachedTowerData;
}
