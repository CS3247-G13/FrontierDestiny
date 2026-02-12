

#include "ModeComponent.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

UModeComponent::UModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UModeComponent::BeginPlay()
{
	Super::BeginPlay();

	Pawn = Cast<APawn>(GetOwner());

	if (!IsValid(Pawn))
	{
		return;
	}

	Camera = Pawn->GetComponentByClass<UCameraComponent>();

	SetupInput(Pawn->InputComponent);

	PlayerController = Pawn->GetController<APlayerController>();
}

void UModeComponent::TickWhenActive()
{
	// Do nothing by default
}

void UModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsModeActive)
	{
		TickWhenActive();
	}
}

void UModeComponent::ActivateMode()
{
	bIsModeActive = true;

	if (IsValid(PlayerController))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				if (ModeIMC)
				{
					Subsystem->AddMappingContext(ModeIMC, 1);
				}
			}
		}
	}
}

void UModeComponent::DeactivateMode()
{
	bIsModeActive = false;

	if (IsValid(PlayerController))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				if (ModeIMC)
				{
					Subsystem->RemoveMappingContext(ModeIMC);
				}
			}
		}
	}
}

void UModeComponent::SetupInput(UInputComponent* Input)
{
	// Do nothing by default
}

