// Fill out your copyright notice in the Description page of Project Settings.


#include "MainHUD.h"
#include "CoreActor.h"
#include "InteractorComponent.h"
#include "InteractableComponent.h"
#include "Kismet/GameplayStatics.h"

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("BeginPlay from MainHUD"));

	if (PlayerHUDClass)
	{
		PlayerHUD = CreateWidget<UPlayerHUDWidget>(
			GetWorld(),
			PlayerHUDClass
		);

		if (PlayerHUD)
		{
			PlayerHUD->AddToViewport();
		}
	}

	// Find Cores and bind to multicast
	TArray<AActor*> FoundCores;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ACoreActor::StaticClass(),
		FoundCores
	);

	for (AActor* Actor : FoundCores)
	{
		ACoreActor* Core = Cast<ACoreActor>(Actor);
		if (Core)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Binding instance %s | Address: %p"),
				*Core->GetName(),
				Core
			);

			Core->OnCoreActivated.AddDynamic(
				this,
				&AMainHUD::HandleCoreActivated
			);

			Core->OnCoreHPChanged.AddDynamic(
				this,
				&AMainHUD::HandleCoreHPChanged
			);
		}
	}

	// Find player and bind
	APawn* Pawn = GetOwningPawn();
	if (!Pawn) return;

	UInteractorComponent* Interactor = Pawn->FindComponentByClass<UInteractorComponent>();

	if (Interactor)
	{
		Interactor->OnInteractableFocused.AddDynamic(
			this,
			&AMainHUD::HandleInteractableFocused
		);

		Interactor->OnInteractableLost.AddDynamic(
			this,
			&AMainHUD::HandleInteractableLost
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("Found %d cores from MainHUD"), FoundCores.Num());
}

void AMainHUD::HandleCoreHPChanged(ACoreActor* CoreActor)
{
	if (!PlayerHUD || !CoreActor) return;

	int32 Index = CoreActor->CoreIndex;
	PlayerHUD->UpdateCore(Index, CoreActor->CoreData);
}

void AMainHUD::HandleCoreActivated(ACoreActor* CoreActor)
{
	UE_LOG(LogTemp, Warning, TEXT("Core %d Activated from MainHUD"), CoreActor->CoreIndex);
	if (!PlayerHUD || !CoreActor) return;

	PlayerHUD->ActivateCore(
		CoreActor->CoreIndex,
		CoreActor->CoreData
	);
}

void AMainHUD::HandleInteractableFocused(UInteractableComponent* Interactable)
{
	if (PlayerHUD && Interactable)
	{
		PlayerHUD->ShowInteractText(
			Interactable->InteractionText
		);
	}
}

void AMainHUD::HandleInteractableLost()
{
	if (PlayerHUD)
	{
		PlayerHUD->HideInteractText();
	}
}