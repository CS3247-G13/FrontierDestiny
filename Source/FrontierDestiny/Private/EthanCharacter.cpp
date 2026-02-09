// Fill out your copyright notice in the Description page of Project Settings.


#include "EthanCharacter.h"

// Sets default values
AEthanCharacter::AEthanCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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
			Subsystem->AddMappingContext(FirstPersonContext, 0);
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("We are using AdventureCharacter."));
}

// Called every frame
void AEthanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEthanCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// MovePlayer
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEthanCharacter::MovePlayer);

		// MoveCamera
		EnhancedInputComponent->BindAction(CameraAction, ETriggerEvent::Triggered, this, &AEthanCharacter::MoveCamera);

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

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
		AddControllerYawInput(CameraVector.X);
		AddControllerPitchInput(CameraVector.Y);
	}
}