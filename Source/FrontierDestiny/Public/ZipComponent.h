// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZipComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class ACoreActor;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FRONTIERDESTINY_API UZipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZipComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputMappingContext> ZipIMC;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UInputAction> ZipAction;

	// How fast ZipProgress moves per second (0→1)
	UPROPERTY(EditAnywhere, Category = "Setup")
	float ZipProgressSpeed = 1.f;

	// FOV when fully zoomed in (ZipProgress == 1)
	UPROPERTY(EditAnywhere, Category = "Setup")
	float ZoomedFOV = 50.f;

	// Normal FOV to restore on zip end
	UPROPERTY(EditAnywhere, Category = "Setup")
	float DefaultFOV = 90.f;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	float ZipProgress = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	bool bIsZipping = false;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TObjectPtr<ACoreActor> HoveredCore;

protected:
	UPROPERTY()
	TObjectPtr<APawn> Pawn;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	UFUNCTION()
	void OnZipStarted();

	UFUNCTION()
	void OnZipEnded();
};
