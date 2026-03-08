// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ElectricFence.generated.h"

class UBoxComponent;
class UNiagaraSystem;

UCLASS()
class FRONTIERDESTINY_API AElectricFence : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AElectricFence();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// The visual mesh for the fence
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fence Components")
	UStaticMeshComponent* FenceMesh;

	// The invisible trigger volume for damage
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fence Components")
	UBoxComponent* DamageZone;

	// Pointer to our Niagara spark system, assignable in the Editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence Effects")
	UNiagaraSystem* SparkEffect;

	// How much damage the fence does per zap
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence Stats")
	float DamageAmount = 10.0f;

	// The function we will bind to the overlap event
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
