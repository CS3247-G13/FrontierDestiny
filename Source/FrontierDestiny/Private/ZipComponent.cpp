// Fill out your copyright notice in the Description page of Project Settings.

#include "ZipComponent.h"
#include "CoreActor.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "EngineUtils.h"
#include "CustomChannels.h"

UZipComponent::UZipComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UZipComponent::BeginPlay()
{
	Super::BeginPlay();

	Pawn = Cast<APawn>(GetOwner());
	if (!IsValid(Pawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("ZipComponent: Owner is not a Pawn"));
		return;
	}

	Camera = Pawn->GetComponentByClass<UCameraComponent>();

	// Register IMC directly at priority 0 — always active
	if (APlayerController* PC = Pawn->GetController<APlayerController>())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				if (ZipIMC)
				{
					Subsystem->AddMappingContext(ZipIMC, 0);
					UE_LOG(LogTemp, Log, TEXT("ZipComponent: Registered IMC %s"), *ZipIMC->GetName());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("ZipComponent: ZipIMC is not assigned"));
				}
			}
		}
	}

	// Bind input actions
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(Pawn->InputComponent))
	{
		if (ZipAction)
		{
			EIC->BindAction(ZipAction, ETriggerEvent::Started,   this, &UZipComponent::OnZipStarted);
			EIC->BindAction(ZipAction, ETriggerEvent::Completed, this, &UZipComponent::OnZipEnded);
			EIC->BindAction(ZipAction, ETriggerEvent::Canceled,  this, &UZipComponent::OnZipEnded);
			UE_LOG(LogTemp, Log, TEXT("ZipComponent: Input bindings set up successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ZipComponent: ZipAction is not assigned"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ZipComponent: Failed to get EnhancedInputComponent"));
	}
}

void UZipComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsZipping)
	{
		return;
	}

	// Raycast from camera to find a hovered core
	ACoreActor* NewHoveredCore = nullptr;
	if (IsValid(Camera))
	{
		FHitResult Hit;
		const FVector TraceStart = Camera->GetComponentLocation();
		const FVector TraceEnd   = TraceStart + Camera->GetForwardVector() * 200000.f;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Pawn);

		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, CC_Core, Params))
		{
			NewHoveredCore = Cast<ACoreActor>(Hit.GetActor());
		}
	}

	// Update hovered core if it changed
	if (NewHoveredCore != HoveredCore)
	{
		if (IsValid(HoveredCore))
		{
			UE_LOG(LogTemp, Log, TEXT("ZipComponent: Unhovered core %s"), *HoveredCore->GetName());
			HoveredCore->SetHovered(false);
		}
		HoveredCore = NewHoveredCore;
		if (IsValid(HoveredCore))
		{
			UE_LOG(LogTemp, Log, TEXT("ZipComponent: Hovered core %s"), *HoveredCore->GetName());
			HoveredCore->SetHovered(true);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("ZipComponent: No core hovered"));
		}
	}

	// Increment progress while hovering a core, decrement otherwise
	if (IsValid(HoveredCore))
	{
		ZipProgress = FMath::Min(ZipProgress + ZipProgressSpeed * DeltaTime, 1.f);
	}
	else
	{
		ZipProgress = FMath::Max(ZipProgress - ZipProgressSpeed * DeltaTime, 0.f);
	}

	// Drive camera FOV from ZipProgress
	if (IsValid(Camera))
	{
		Camera->SetFieldOfView(FMath::Lerp(DefaultFOV, ZoomedFOV, ZipProgress));
	}

	// Teleport when fully zoomed in
	if (ZipProgress >= 1.f && IsValid(HoveredCore) && IsValid(Pawn))
	{
		UE_LOG(LogTemp, Log, TEXT("ZipComponent: Teleporting to core %s at %s"),
			*HoveredCore->GetName(), *HoveredCore->GetActorLocation().ToString());
		Pawn->SetActorLocation(HoveredCore->GetActorLocation());
		OnZipEnded();
	}
}

void UZipComponent::OnZipStarted()
{
	UE_LOG(LogTemp, Log, TEXT("ZipComponent: Zip started"));
	bIsZipping = true;

	int32 CoreCount = 0;
	for (ACoreActor* Core : TActorRange<ACoreActor>(GetWorld()))
	{
		Core->SetShownThroughWalls(true);
		CoreCount++;
	}
	UE_LOG(LogTemp, Log, TEXT("ZipComponent: Showing %d cores through walls"), CoreCount);
}

void UZipComponent::OnZipEnded()
{
	UE_LOG(LogTemp, Log, TEXT("ZipComponent: Zip ended"));
	bIsZipping  = false;
	ZipProgress = 0.f;

	if (IsValid(HoveredCore))
	{
		HoveredCore->SetHovered(false);
		HoveredCore = nullptr;
	}

	for (ACoreActor* Core : TActorRange<ACoreActor>(GetWorld()))
	{
		Core->SetShownThroughWalls(false);
	}

	if (IsValid(Camera))
	{
		Camera->SetFieldOfView(DefaultFOV);
	}
}
