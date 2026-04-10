// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "EthanCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UInputComponent;
class ACoreActor;
class USoundBase;

UCLASS()
class FRONTIERDESTINY_API AEthanCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEthanCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputMappingContext> FirstPersonIMC;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> CameraAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> ZipAction;

	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector2D CameraSensitivity = FVector2D(1.f, 1.f);

	// How fast ZipProgress moves per second (0→1)
	UPROPERTY(EditAnywhere, Category = "Zip")
	float ZipProgressSpeed = 1.f;

	// FOV when fully zoomed in
	UPROPERTY(EditAnywhere, Category = "Zip")
	float ZoomedFOV = 50.f;

	// Normal FOV to restore on zip end
	UPROPERTY(EditAnywhere, Category = "Zip")
	float DefaultFOV = 90.f;

	// Sound played when the player teleports to a core
	UPROPERTY(EditAnywhere, Category = "Zip")
	TObjectPtr<USoundBase> ZipTeleportSound;

	UPROPERTY(VisibleAnywhere, Category = "Zip|Debug")
	float ZipProgress = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Zip|Debug")
	bool bIsZipping = false;

	UPROPERTY(VisibleAnywhere, Category = "Zip|Debug")
	TObjectPtr<ACoreActor> HoveredCore;

	UFUNCTION()
	void OnZipStarted();

	UFUNCTION()
	void OnZipEnded();

	UFUNCTION(BlueprintImplementableEvent)
	void OnZip(int CoreIndex);

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void MovePlayer(const FInputActionValue& Value);

	UFUNCTION()
	void MoveCamera(const FInputActionValue& Value);
};
