// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlayerHUDWidget.h"
#include "MainHUD.generated.h"

class UPlayerHUDWidget;
class ACoreActor;

UCLASS()
class FRONTIERDESTINY_API AMainHUD : public AHUD
{
	GENERATED_BODY()

protected:
	
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerHUDWidget> PlayerHUDClass;

private:
	UPROPERTY()
	UPlayerHUDWidget* PlayerHUD;

	UPROPERTY()
	TArray<ACoreActor*> Cores;

	UFUNCTION()
	void HandleCoreHPChanged(ACoreActor* CoreActor);

	UFUNCTION()
	void HandleCoreActivated(ACoreActor* CoreActor);

	UFUNCTION()
	void HandleInteractableFocused(UInteractableComponent* Interactable);

	UFUNCTION()
	void HandleInteractableLost();
};
