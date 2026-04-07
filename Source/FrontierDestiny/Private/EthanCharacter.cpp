// Fill out your copyright notice in the Description page of Project Settings.

#include "EthanCharacter.h"
#include "CoreActor.h"
#include "CustomChannels.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AEthanCharacter::AEthanCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AEthanCharacter::BeginPlay()
{
	Super::BeginPlay();
	check(GEngine != nullptr);

	// Convert Controller to Enhanced Input Local Player Subsystem and Update with Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(FirstPersonIMC, 0);
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("We are using AdventureCharacter."));
}

// Called every frame
void AEthanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsZipping && ZipProgress <= 0.f)
	{
		return;
	}

	UCameraComponent* Camera = GetComponentByClass<UCameraComponent>();

	if (bIsZipping)
	{
		// Raycast from camera to find a hovered core
		ACoreActor* NewHoveredCore = nullptr;
		if (IsValid(Camera))
		{
			FHitResult Hit;
			const FVector TraceStart = Camera->GetComponentLocation();
			const FVector TraceEnd   = TraceStart + Camera->GetForwardVector() * 200000.f;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, CC_Core, Params))
			{
				ACoreActor* HitCore = Cast<ACoreActor>(Hit.GetActor());
				if (IsValid(HitCore) && HitCore->bIsCoreActive)
				{
					NewHoveredCore = HitCore;
				}
			}
		}

		// Update hovered core if it changed
		if (NewHoveredCore != HoveredCore)
		{
			if (IsValid(HoveredCore))
			{
				HoveredCore->SetHovered(false);
			}
			HoveredCore = NewHoveredCore;
			if (IsValid(HoveredCore))
			{
				HoveredCore->SetHovered(true);
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

		// Teleport when fully zoomed in
		if (ZipProgress >= 1.f && IsValid(HoveredCore))
		{
			SetActorLocation(HoveredCore->GetTeleportPoint());
			if (ZipTeleportSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, ZipTeleportSound, GetActorLocation());
			}
			OnZipEnded();
			return;
		}
	}
	else
	{
		// Button released — wind progress back down
		ZipProgress = FMath::Max(ZipProgress - ZipProgressSpeed * DeltaTime, 0.f);
	}

	// Drive camera FOV from ZipProgress (both during zip and wind-down)
	if (IsValid(Camera))
	{
		Camera->SetFieldOfView(FMath::Lerp(DefaultFOV, ZoomedFOV, ZipProgress));
	}
}

// Called to bind functionality to input
void AEthanCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction,   ETriggerEvent::Triggered, this, &AEthanCharacter::MovePlayer);
		EnhancedInputComponent->BindAction(CameraAction, ETriggerEvent::Triggered, this, &AEthanCharacter::MoveCamera);
		EnhancedInputComponent->BindAction(JumpAction,   ETriggerEvent::Started,   this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction,   ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		if (ZipAction)
		{
			EnhancedInputComponent->BindAction(ZipAction, ETriggerEvent::Started,   this, &AEthanCharacter::OnZipStarted);
			EnhancedInputComponent->BindAction(ZipAction, ETriggerEvent::Completed, this, &AEthanCharacter::OnZipEnded);
			EnhancedInputComponent->BindAction(ZipAction, ETriggerEvent::Canceled,  this, &AEthanCharacter::OnZipEnded);
		}
	}
}

void AEthanCharacter::MovePlayer(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FVector Right = GetActorRightVector();
		AddMovementInput(Right, MovementVector.X);

		const FVector Forward = GetActorForwardVector();
		AddMovementInput(Forward, MovementVector.Y);
	}
}

void AEthanCharacter::MoveCamera(const FInputActionValue& Value)
{
	const FVector2D CameraVector = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(CameraVector.X * CameraSensitivity.X);
		AddControllerPitchInput(CameraVector.Y * CameraSensitivity.Y);
	}
}

void AEthanCharacter::OnZipStarted()
{
	bIsZipping = true;

	for (ACoreActor* Core : TActorRange<ACoreActor>(GetWorld()))
	{
		if (Core->bIsCoreActive)
		{
			Core->SetShownThroughWalls(true);
		}
	}
}

void AEthanCharacter::OnZipEnded()
{
	bIsZipping = false;

	if (IsValid(HoveredCore))
	{
		HoveredCore->SetHovered(false);
		HoveredCore = nullptr;
	}

	for (ACoreActor* Core : TActorRange<ACoreActor>(GetWorld()))
	{
		Core->SetShownThroughWalls(false);
	}
}
