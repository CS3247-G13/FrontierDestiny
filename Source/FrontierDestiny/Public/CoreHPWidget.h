// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoreData.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "CoreHPWidget.generated.h"

UCLASS()
class FRONTIERDESTINY_API UCoreHPWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable)
	void InitializeCoreBar(const FCoreData& CoreData);
	
	UFUNCTION(BlueprintCallable)
	void UpdateHP(const FCoreData& CoreData);


protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UImage* HP_Bar;

	UPROPERTY(meta = (BindWidget))
	UImage* Icon;

	UPROPERTY()
	UMaterialInstanceDynamic* HealthBarMat;
};
