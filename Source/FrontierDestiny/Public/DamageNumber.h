// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DamageNumber.generated.h"

class UTextRenderComponent;

UCLASS()
class FRONTIERDESTINY_API ADamageNumber : public AActor
{
	GENERATED_BODY()

public:
	ADamageNumber();

	UPROPERTY(EditAnywhere, Category = "Setup")
	int32 DamageNumber = 50;

	void SetDamageNumber(int32 Value);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY(EditAnywhere, Category = "Setup")
	float Lifetime = 2.f;

	UPROPERTY(EditAnywhere, Category = "Setup")
	float RiseSpeed = 60.f;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	float Progress = 0.f;

public:
	virtual void Tick(float DeltaTime) override;
};
