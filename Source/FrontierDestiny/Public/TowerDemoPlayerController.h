// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TowerDemoPlayerController.generated.h"

class UInputAction;
struct FInputActionValue;

class UBuilderComponent;
class UTowerData;

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API ATowerDemoPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/*
	The main HUD widget class*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainHUDWidget;
	/*
	The Input action that maps to building the tower*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TObjectPtr<UInputAction> BuildInputAction;
	/*
	The Input action that maps to rotating the tower*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TObjectPtr<UInputAction> RotateInputAction;
	/*
	The input action that maps to the number keys*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TObjectPtr<UInputAction> NumberKeyInputAction;
	/*
	The input action that maps to enabling and disabling build mode*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TObjectPtr<UInputAction> ToggleBuildModeInputAction;
	/*
	Array of Tower Data that the player can use*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TArray<TObjectPtr<UTowerData>> AvailableTowers;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY()
	TObjectPtr<UBuilderComponent> BuilderComponent;

	UFUNCTION()
	void OnBuildAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnRotateAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnNumberKeyAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnToggleBuildModeAction(const FInputActionValue& Value);
};
