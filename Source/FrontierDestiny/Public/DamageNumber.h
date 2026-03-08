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
	// Sets default values for this actor's properties
	ADamageNumber();

	UPROPERTY(EditAnywhere, Category = "Setup")
	int32 DamageNumber = 50;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY(EditAnywhere, Category = "Setup")
	float Lifetime = 1.f;


	UPROPERTY(VisibleAnywhere, Category = "Debug")
	float Progress = 0.f;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
