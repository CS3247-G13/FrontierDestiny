// Fill out your copyright notice in the Description page of Project Settings.

#include "EditorComponent.h"
#include "EconomyComponent.h"
#include "TowerActor.h"
#include "TowerDemoPlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "TowerData.h"
#include "GridActor.h"

#define LOG(Message) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Message)

UEditorComponent::UEditorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEditorComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeComponentReferences();
	InitializeOutlineMaterial();
}

void UEditorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsComponentActive)
	{
		SelectAndHighlightTower(nullptr);
		return;
	}

	if (!bIsSelectionLocked)
	{
		FHitResult Hit;
		if (!TryPerformRaycast(Hit))
		{
			SelectAndHighlightTower(nullptr);
			return;
		}

		ATowerActor* HitTower = Cast<ATowerActor>(Hit.GetActor());
		if (!IsValid(HitTower))
		{
			SelectAndHighlightTower(nullptr);
			return;
		}

		SelectAndHighlightTower(HitTower);
	}
	
	if (IsValid(SelectedTower))
	{
		if (FVector::Dist(SelectedTower->GetActorLocation(), GetOwner()->GetActorLocation()) > InteractionRange)
		{
			SelectAndHighlightTower(nullptr);
		}
	}
}

void UEditorComponent::ActivateEditorMode()
{
	bIsComponentActive = true;
}

void UEditorComponent::DeactivateEditorMode()
{
	bIsComponentActive = false;
	bIsSelectionLocked = false;
	SelectAndHighlightTower(nullptr);
}

void UEditorComponent::InitializeComponentReferences()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	EconomyComponent = OwnerPawn->GetController()->GetComponentByClass<UEconomyComponent>();
}

void UEditorComponent::InitializeOutlineMaterial() 
{
	if (OutlineMaterialBase)
	{
		OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterialBase, this);

		UCameraComponent* PlayerCamera = Cast<APawn>(GetOwner())->FindComponentByClass<UCameraComponent>();
		if (PlayerCamera && OutlineMID)
		{
			// Access the settings directly
			FPostProcessSettings& Settings = PlayerCamera->PostProcessSettings;

			// Add your Dynamic Material to the blendables
			Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, OutlineMID));

			UpdateOutlineColor(FLinearColor(1.f, 0.306f, 0.f, 1.f));
		}
	}
}

void UEditorComponent::SelectAndHighlightTower(ATowerActor* ActorToHighlight)
{
	if (ActorToHighlight == SelectedTower)
	{
		return;
	}
	if (IsValid(SelectedTower))
	{
		// Clear the current highlighted actor
		TArray<UStaticMeshComponent*> MeshComponents;
		SelectedTower->GetComponents<UStaticMeshComponent>(MeshComponents);

		for (UStaticMeshComponent* Mesh : MeshComponents)
		{
			Mesh->SetRenderCustomDepth(false);
		}
	}
	if (!IsValid(ActorToHighlight))
	{
		UpdateSelectedTower(nullptr);
		return;
	}

	UpdateSelectedTower(ActorToHighlight);
	TArray<UStaticMeshComponent*> NewMeshComponents;
	SelectedTower->GetComponents<UStaticMeshComponent>(NewMeshComponents);

	for (UStaticMeshComponent* Mesh : NewMeshComponents)
	{
		Mesh->SetRenderCustomDepth(true);
		Mesh->SetCustomDepthStencilValue(37);
	}
}

bool UEditorComponent::TryPerformRaycast(FHitResult& Hit)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return false;

	UCameraComponent* Camera = OwnerPawn->FindComponentByClass<UCameraComponent>();
	if (!Camera) return false;

	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + (Camera->GetForwardVector() * InteractionRange);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(OwnerPawn);

	return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, TraceParams);
}

void UEditorComponent::UpdateOutlineColor(FLinearColor NewColor)
{
	if (OutlineMID)
	{
		OutlineMID->SetVectorParameterValue(TEXT("Outline Color"), NewColor);
	}
}

void UEditorComponent::LockSelectedTower()
{
	if (!bIsComponentActive)
	{
		return;
	}
	if (SelectedTower)
	{
		bIsSelectionLocked = true;
	}
	else
	{
		// Do nothing, since there is no tower to lock onto
	}
}

void UEditorComponent::UnlockSelectedTower()
{
	if (!bIsComponentActive)
	{
		return;
	}
	bIsSelectionLocked = false;
}

void UEditorComponent::ToggleLock()
{
	if (!bIsComponentActive)
	{
		return;
	}
	if (!bIsSelectionLocked)
	{
		LockSelectedTower();
	}
	else
	{
		UnlockSelectedTower();
	}
}

void UEditorComponent::UpgradeSelectedTower(int32 Index)
{
	if (!bIsComponentActive)
	{
		return;
	}
	if (!SelectedTower || !EconomyComponent) return;

	TArray<UTowerData*> Upgrades = SelectedTower->GetUpgrades();
	if (Index >= Upgrades.Num())
	{
		return;
	}

	UTowerData* Upgrade = Upgrades[Index];
	if (!EconomyComponent->TryDeductFunds(Upgrade->TowerCost))
	{
		return;
	}

	ATowerActor* NewTower = GetWorld()->SpawnActorDeferred<ATowerActor>(
		Upgrade->TowerBlueprint,
		SelectedTower->GetActorTransform(),
		GetOwner(),
		Cast<APawn>(GetOwner()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (NewTower)
	{
		NewTower->bIsGhost = false;
		NewTower->TowerInfo = Upgrade;
		NewTower->FinishSpawning(SelectedTower->GetActorTransform());
		NewTower->GridActor = SelectedTower->GridActor;
		NewTower->CornerGridIndex = SelectedTower->CornerGridIndex;
	}

	if (IsValid(SelectedTower->GridActor))
	{
		SelectedTower->
			GridActor->
			PlaceTower(SelectedTower->CornerGridIndex, SelectedTower->GetActorRotation(), NewTower);
	}
	
	SelectedTower->Destroy();
	SelectAndHighlightTower(NewTower);
}


void UEditorComponent::DeleteSelectedTower()
{
	if (!bIsComponentActive)
	{
		return;
	}
	if (!SelectedTower) return;

	SelectedTower->GridActor->RemoveTower(SelectedTower->CornerGridIndex, SelectedTower->GetActorRotation(), SelectedTower);
	// Logic for refunding goes here before destruction
	SelectedTower->Destroy();
	UpdateSelectedTower(nullptr);
}

void UEditorComponent::UpdateSelectedTower(ATowerActor* NewTower)
{
	SelectedTower = NewTower;

	if (NewTower)
	{
		OnTowerSelected.Broadcast(NewTower->TowerInfo);
	}
	else
	{
		OnTowerSelected.Broadcast(nullptr);
	}
}