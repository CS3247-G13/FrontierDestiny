// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerDemoPlayerController.h"
#include "BuilderComponent.h"
#include "CombatComponent.h"

#include "Blueprint/UserWidget.h"

#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

void ATowerDemoPlayerController::BeginPlay()
{
	Super::BeginPlay();

    InitializeHUD();
    InitializeComponentReferences();
    InitializeIMC();
}

void ATowerDemoPlayerController::InitializeComponentReferences()
{
    if (GetPawn())
    {
        ModeMap.Add(EMode::Builder, GetPawn()->FindComponentByClass<UBuilderComponent>());
        ModeMap.Add(EMode::Combat, GetPawn()->FindComponentByClass<UCombatComponent>());
    }
}

void ATowerDemoPlayerController::InitializeHUD()
{
    // Initialize the widget or other UI elements for the tower demo here
    if (MainHUDWidget)
    {
        UUserWidget* HUDWidgetInstance = CreateWidget<UUserWidget>(this, MainHUDWidget);
        if (IsValid(HUDWidgetInstance))
        {
            HUDWidgetInstance->AddToViewport();
        }
    }
}

void ATowerDemoPlayerController::InitializeIMC()
{
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
        {
            if (DefaultIMC)
            {
                Subsystem->AddMappingContext(DefaultIMC, 0);
            }
        }
    }
}

void ATowerDemoPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Cast the internal InputComponent to the Enhanced version
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        for (auto& KeyValue : ToggleModeInputActions)
        {
            const EMode UpdatedMode = KeyValue.Key;
            UInputAction* Action = KeyValue.Value;

            if (Action)
            {
                EnhancedInputComponent->BindAction(
                    Action,
                    ETriggerEvent::Triggered,
                    this,
                    &ATowerDemoPlayerController::UpdateMode,
                    UpdatedMode
                );
            }
        }
    }
}

void ATowerDemoPlayerController::UpdateMode(EMode UpdatedMode)
{
    if (UpdatedMode == Mode)
    {
        // Set to default mode
        UpdatedMode = EMode::Combat;
    }
    // Deactivate the previous mode
    ModeMap[Mode]->DeactivateMode();
    Mode = UpdatedMode;
    ModeMap[Mode]->ActivateMode();

    OnModeUpdate.Broadcast(UpdatedMode);
}