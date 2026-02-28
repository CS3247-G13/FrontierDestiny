// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoreData.h"
#include "CoreHPWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include <Components/HorizontalBox.h>
#include "PlayerHUDWidget.generated.h"

class UCoreHPWidget;

UCLASS()
class FRONTIERDESTINY_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetAmmo(int32 NewAmmo);

	void InitializeCores(const TArray<FCoreData>& CoreList);

	void ActivateCore(int32 Index, const FCoreData& CoreData);

	void UpdateCore(int32 Index, const FCoreData& CoreData);

	UFUNCTION(BlueprintCallable)
	void ShowInteractText(const FString& Text);

	UFUNCTION(BlueprintCallable)
	void HideInteractText();
	
protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UImage* MinimapImage;

	UPROPERTY(meta = (BindWidget))
	UImage* CrosshairImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* InteractText;

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* CoreHPContainer;

	UPROPERTY(meta = (BindWidget))
	UCoreHPWidget* Core0;

	UPROPERTY(meta = (BindWidget))
	UCoreHPWidget* Core1;

	UPROPERTY(meta = (BindWidget))
	UCoreHPWidget* Core2;

	UPROPERTY(meta = (BindWidget))
	UCoreHPWidget* Core3;

	UPROPERTY(meta = (BindWidget))
	UCoreHPWidget* Core4;

	UPROPERTY()
	TArray<UCoreHPWidget*> CoreSlots;

	UPROPERTY(EditDefaultsOnly, Category="core")
	TSubclassOf<UCoreHPWidget> CoreHPClass;
};
