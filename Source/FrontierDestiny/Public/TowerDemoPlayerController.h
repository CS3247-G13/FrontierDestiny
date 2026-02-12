// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TowerDemoPlayerController.generated.h"

class UInputAction;
struct FInputActionValue;
class UInputMappingContext;

class UTowerData;
class UBuilderComponent;
class UEditorComponent;

UENUM(BlueprintType)
enum class EMode : uint8 
{
	Builder	UMETA(DisplayName = "Builder"),
	Editor	UMETA(DisplayName = "Editor"),
	None	UMETA(DisplayName = "None")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModeUpdate, EMode, Mode);

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API ATowerDemoPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/*
	The input mapping context*/
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> TowerDemoInputMappingContext;

	/*
	The main HUD widget class*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainHUDWidget;
	/*
	The input action that maps to the number keys*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Building")
	TObjectPtr<UInputAction> NumberKeyInputAction;

	// BUILDING
	/*
	The Input action that maps to building the tower*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Building")
	TObjectPtr<UInputAction> BuildInputAction;
	/*
	The Input action that maps to rotating the tower*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Building")
	TObjectPtr<UInputAction> RotateInputAction;
	/*
	The input action that maps to enabling and disabling build mode*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Building")
	TObjectPtr<UInputAction> ToggleBuildModeInputAction;

	// EDITOR
	/*
	The input action that maps to enabling and disabling editor mode*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Editing")
	TObjectPtr<UInputAction> ToggleEditModeInputAction;
	/*
	The input action that maps to locking the selection*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Editing")
	TObjectPtr<UInputAction> LockSelectionAction;
	/*
	The input action that maps to deleting the selection*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Editing")
	TObjectPtr<UInputAction> DeleteSelectionAction;

	/*
	Array of Tower Data that the player can use*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower")
	TArray<TObjectPtr<UTowerData>> AvailableTowers;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnModeUpdate OnModeUpdate;
protected:
	virtual void BeginPlay() override;
	void InitializeComponentReferences();
	void InitializeHUD();
	void InitializeInputMappingContext();
	virtual void SetupInputComponent() override;

	EMode Mode = EMode::None;

	// ==== BUILDER ====
	UPROPERTY()
	TObjectPtr<UBuilderComponent> BuilderComponent;

	// Build tower
	UFUNCTION()
	void OnBuildAction(const FInputActionValue& Value);
	// Rotate tower
	UFUNCTION()
	void OnRotateAction(const FInputActionValue& Value);
	// Select a tower
	UFUNCTION()
	void OnNumberKeyAction(const FInputActionValue& Value);
	// Toggle build mode
	UFUNCTION()
	void OnToggleBuildModeAction(const FInputActionValue& Value);

	void UpdateMode(EMode UpdatedMode);
	
	// ==== EDITOR ====
	UPROPERTY()
	TObjectPtr<UEditorComponent> EditorComponent;

	UFUNCTION()
	void OnSelectAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnUpgradeAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnDeleteAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnToggleEditorModeAction(const FInputActionValue& Value);
	
};
