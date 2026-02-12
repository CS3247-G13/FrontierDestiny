// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerDemoPlayerController.h"
#include "BuilderComponent.h"
#include "EditorComponent.h"

#include "Blueprint/UserWidget.h"

#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

void ATowerDemoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
    InitializeInputMappingContext();
    InitializeHUD();
    InitializeComponentReferences();
}

void ATowerDemoPlayerController::InitializeComponentReferences()
{
    if (GetPawn())
    {
        BuilderComponent = GetPawn()->FindComponentByClass<UBuilderComponent>();
        EditorComponent = GetPawn()->FindComponentByClass<UEditorComponent>();
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

void ATowerDemoPlayerController::InitializeInputMappingContext()
{
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
        {
            if (TowerDemoInputMappingContext)
            {
                Subsystem->AddMappingContext(TowerDemoInputMappingContext, 0);
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
        if (NumberKeyInputAction)
        {
            EnhancedInputComponent->BindAction(
                NumberKeyInputAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnNumberKeyAction
            );
            
            EnhancedInputComponent->BindAction(
                NumberKeyInputAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnUpgradeAction
            );
            
        }

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

        if (ToggleBuildModeInputAction)
        {
            EnhancedInputComponent->BindAction(
                ToggleBuildModeInputAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnToggleBuildModeAction
            );
		}

        if (ToggleEditModeInputAction)
        {
            EnhancedInputComponent->BindAction(
                ToggleEditModeInputAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnToggleEditorModeAction
            );
        }

        if (LockSelectionAction)
        {
            EnhancedInputComponent->BindAction(
                LockSelectionAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnSelectAction
            );
        }

        if (DeleteSelectionAction)
        {
            EnhancedInputComponent->BindAction(
                DeleteSelectionAction,
                ETriggerEvent::Started,
                this,
                &ATowerDemoPlayerController::OnDeleteAction
            );
        }
    }
}

void ATowerDemoPlayerController::UpdateMode(EMode UpdatedMode)
{
    Mode = UpdatedMode;
    OnModeUpdate.Broadcast(UpdatedMode);
}

// ======== BUILDING INPUT ACTIONS ======== //

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
	int32 KeyNumber = FMath::RoundToInt(Value.Get<float>());
	if (KeyNumber <= AvailableTowers.Num() && KeyNumber > 0)
    {
		BuilderComponent->ChangeTowerSelection(AvailableTowers[KeyNumber - 1]);
    }
}

void ATowerDemoPlayerController::OnToggleBuildModeAction(const FInputActionValue& Value)
{
    if (Mode != EMode::Builder)
    {
        BuilderComponent->ActivateBuildingMode();
        EditorComponent->DeactivateEditorMode();
        UpdateMode(EMode::Builder);
    }
    else
    {
        BuilderComponent->DeactivateBuildingMode();
        UpdateMode(EMode::None);
    }
}


// ======== EDITOR INPUT ACTIONS ======== //

void ATowerDemoPlayerController::OnSelectAction(const FInputActionValue& Value)
{
    EditorComponent->ToggleLock();
}

void ATowerDemoPlayerController::OnUpgradeAction(const FInputActionValue& Value)
{
    EditorComponent->UpgradeSelectedTower(FMath::RoundToInt(Value.Get<float>()) - 1);
}

void ATowerDemoPlayerController::OnDeleteAction(const FInputActionValue& Value)
{
    EditorComponent->DeleteSelectedTower();
}

void ATowerDemoPlayerController::OnToggleEditorModeAction(const FInputActionValue& Value)
{
    if (Mode != EMode::Editor)
    {
        EditorComponent->ActivateEditorMode();
        BuilderComponent->DeactivateBuildingMode();
        UpdateMode(EMode::Editor);
    }
    else
    {
        EditorComponent->DeactivateEditorMode();
        UpdateMode(EMode::None);
    }
}
