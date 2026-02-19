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
class UModeComponent;

UENUM(BlueprintType)
enum class EMode : uint8 
{
	Builder	UMETA(DisplayName = "Builder"),
	Combat	UMETA(DisplayName = "Combat")
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
	TObjectPtr<UInputMappingContext> DefaultIMC;

	/*
	The main HUD widget class*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainHUDWidget;

	/*
	The input action that maps to enabling and disabling editor mode*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower")
	TMap<EMode, UInputAction*> ToggleModeInputActions;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnModeUpdate OnModeUpdate;
protected:
	virtual void BeginPlay() override;
	void InitializeComponentReferences();
	void InitializeHUD();
	void InitializeIMC();

	virtual void SetupInputComponent() override;

	EMode Mode = EMode::Combat;

	UPROPERTY()
	TObjectPtr<UBuilderComponent> BuilderComponent;

	void UpdateMode(EMode UpdatedMode);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<EMode, UModeComponent*> ModeMap;
};
