// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerDemoPlayerController.h"
#include "BuilderComponent.h"

#include "Blueprint/UserWidget.h"

#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#define LOG(Message) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Message)

void ATowerDemoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize the widget or other UI elements for the tower demo here

	if (MainHUDWidget)
	{
		UUserWidget* HUDWidgetInstance = CreateWidget<UUserWidget>(this, MainHUDWidget);
		if (IsValid(HUDWidgetInstance))
		{
			HUDWidgetInstance->AddToViewport();
		}
	}

    if (!(BuilderComponent = Cast<UBuilderComponent>(GetPawn()->GetComponentByClass(UBuilderComponent::StaticClass())))) 
    {
        UE_LOG(LogTemp, Error, 
			TEXT("TowerDemoPlayerController on %s failed to find BuilderComponent. Are you sure the BuilderComponent is attached to the PlayerController's Pawn?"), *GetName());
    }
}

void ATowerDemoPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Cast the internal InputComponent to the Enhanced version
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // Bind the "Build" action
        if (BuildInputAction)
        {
            EnhancedInputComponent->BindAction(
                BuildInputAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnBuildAction
            );
        }

        if (RotateInputAction)
        {
            EnhancedInputComponent->BindAction(
                RotateInputAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnRotateAction
            );
		}

        if (NumberKeyInputAction)
        {
            EnhancedInputComponent->BindAction(
                NumberKeyInputAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnNumberKeyAction
            );
        }

        if (ToggleBuildModeInputAction)
        {
            EnhancedInputComponent->BindAction(
                ToggleBuildModeInputAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnToggleBuildModeAction
            );
		}
    }
}

void ATowerDemoPlayerController::OnBuildAction(const FInputActionValue& Value)
{
	BuilderComponent->TryBuildTower();
}

void ATowerDemoPlayerController::OnRotateAction(const FInputActionValue& Value)
{
    BuilderComponent->RotateTower(Value.Get<float>() > 0);
}

void ATowerDemoPlayerController::OnNumberKeyAction(const FInputActionValue& Value)
{
    BuilderComponent->ActivateBuildingMode();
	int32 KeyNumber = FMath::RoundToInt(Value.Get<float>());
	if (KeyNumber <= AvailableTowers.Num() && KeyNumber > 0)
    {
		BuilderComponent->ChangeTowerSelection(AvailableTowers[KeyNumber - 1]);
    }
}

void ATowerDemoPlayerController::OnToggleBuildModeAction(const FInputActionValue& Value)
{
	BuilderComponent->ToggleBuildingMode();
}
