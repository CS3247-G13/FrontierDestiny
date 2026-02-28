// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoreData.h"
#include "InteractableComponent.h"
#include "CoreActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoreHPChanged, ACoreActor*, UpdatedCoreData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoreActivated, ACoreActor*, CoreActor);

UCLASS()
class FRONTIERDESTINY_API ACoreActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ACoreActor();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	FCoreData CoreData;

	UPROPERTY(BlueprintAssignable, Category="Core")
	FOnCoreHPChanged OnCoreHPChanged;

	UPROPERTY(BlueprintAssignable, Category="Core")
	FOnCoreActivated OnCoreActivated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	int32 CoreIndex = 0;

	UFUNCTION(BlueprintCallable)
	void ApplyDamage(float Damage);
	
	UFUNCTION(BlueprintCallable)
	void ActivateCore();

	UPROPERTY(VisibleAnywhere)
	UInteractableComponent* InteractableComponent;

	UFUNCTION()
	void HandleInteraction(AActor* Interactor);


protected:
	virtual void BeginPlay() override;

};
