// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "EthanCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UInputComponent;

UCLASS()
class FRONTIERDESTINY_API AEthanCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEthanCharacter();
	
	// First Person animations
	UPROPERTY(EditAnywhere, Category = Animation)
	TObjectPtr<UAnimBlueprint> FirstPersonDefaultAnim;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputMappingContext> FirstPersonContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> CameraAction;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void MovePlayer(const FInputActionValue& Value);

	UFUNCTION()
	void MoveCamera(const FInputActionValue& Value);
	
	// First Person camera
	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;
 
	// Offset for the first-person camera
	UPROPERTY(EditAnywhere, Category = Camera)
	FVector FirstPersonCameraOffset = FVector(0.0f, 0.0f, 8.0f);
 
	// First-person primitives field of view
	UPROPERTY(EditAnywhere, Category = Camera)
	float FirstPersonFieldOfView = 70.0f;
 
	// First-person primitives view scale
	UPROPERTY(EditAnywhere, Category = Camera)
	float FirstPersonScale = 0.6f;
 
	// First-person mesh, visible only to the owning player
	UPROPERTY(VisibleAnywhere, Category = Mesh)
	TObjectPtr<USkeletalMeshComponent> FirstPersonMeshComponent;
	
private:
	void ConstructFirstPersonMesh();
	void ConstructCamera();
};
