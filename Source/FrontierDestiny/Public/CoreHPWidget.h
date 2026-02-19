// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoreData.h"
#include "Components/ProgressBar.h"
#include "CoreHPWidget.generated.h"

UCLASS()
class FRONTIERDESTINY_API UCoreHPWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	void InitializeCoreBar(const FCoreData& CoreData);

	void UpdateHP(const FCoreData& CoreData);


protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HPBar;
};
