// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TowerDemoPlayerController.generated.h"


class UInputAction;
struct FInputActionValue;
class UInputMappingContext;

class UModeComponent;

UENUM(BlueprintType)
enum class EMode : uint8 
{
	Builder	UMETA(DisplayName = "Builder"),
	Combat	UMETA(DisplayName = "Combat")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModeUpdate, EMode, Mode);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeMenuOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeMenuClosed);
/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API ATowerDemoPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputMappingContext> DefaultIMC;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TSubclassOf<UUserWidget> MainHUDWidget;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TMap<EMode, UInputAction*> ToggleModeInputActions;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> OpenUpgradeMenuAction;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnModeUpdate OnModeUpdate;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnUpgradeMenuOpened OnUpgradeMenuOpened;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnUpgradeMenuClosed OnUpgradeMenuClosed;

	UFUNCTION(BlueprintCallable)
	void CloseUpgradeMenu();

	UFUNCTION(BlueprintCallable)
	void FreeMouse(UUserWidget *WidgetToFocus);

	UFUNCTION(BlueprintCallable)
	void LockMouse();

protected:
	virtual void BeginPlay() override;
	void InitializeComponentReferences();
	void InitializeHUD();
	void InitializeIMC();

	virtual void SetupInputComponent() override;

	EMode Mode = EMode::Combat;
	bool bIsUpgradeMenuOpen = false;

	UFUNCTION()
	void UpdateMode(EMode UpdatedMode);
	UFUNCTION()
	void OnOpenUpgradeMenuAction(const FInputActionValue& Value);
	
	void OpenUpgradeMenu();
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<EMode, UModeComponent*> ModeMap;
};
