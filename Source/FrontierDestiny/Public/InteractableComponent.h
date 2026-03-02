// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteracted, AActor*, Interactor);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FRONTIERDESTINY_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractableComponent();

	void Interact(AActor* Interactor);

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Setup")
	bool bIsOneTimeUse;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Setup")
	bool bIsInteractable = true;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteracted OnInteracted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FString InteractionText = "Press E to Interact";
	
	UFUNCTION(BlueprintCallable)
	void ActivateInteractable();

	UFUNCTION(BlueprintCallable)
	void DeactivateInteractable();

	UFUNCTION(BlueprintCallable)
	bool GetIsInteractable();
};
