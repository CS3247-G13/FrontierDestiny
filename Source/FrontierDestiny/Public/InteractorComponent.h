// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractorComponent.generated.h"

class UInteractableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnInteractableFocused,
	UInteractableComponent*, Interactable
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FOnInteractableLost
);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FRONTIERDESTINY_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable, Category="Interactor")
	void Interact();

	UPROPERTY(BlueprintAssignable, Category = "Interactor")
	FOnInteractableFocused OnInteractableFocused;

	UPROPERTY(BlueprintAssignable, Category = "Interactor")
	FOnInteractableLost OnInteractableLost;

private:
	void CheckForInteractable();

	UPROPERTY()
	UInteractableComponent* CurrentInteractable;

	UInteractableComponent* NewInteractable;

	UPROPERTY(EditAnywhere, Category="Interaction")
	float TraceDistance = 1200.f;
};
