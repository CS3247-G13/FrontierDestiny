// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ModeComponent.generated.h"

class UInputMappingContext;

class UCameraComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FRONTIERDESTINY_API UModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UModeComponent();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	virtual void TickWhenActive();

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable)
	virtual void ActivateMode();

	UFUNCTION(BlueprintCallable)
	virtual void DeactivateMode();

	UFUNCTION()
	virtual void SetupInput(UInputComponent* Input);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> ModeIMC;

protected:
	UPROPERTY()
	TObjectPtr<APawn> Pawn;
	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
	UPROPERTY()
	bool bIsModeActive;
};
