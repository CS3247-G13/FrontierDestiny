// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModeComponent.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UInputAction;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FRONTIERDESTINY_API UCombatComponent : public UModeComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();

	virtual void SetupInput(UInputComponent* InputComponent) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UInputAction> TriggerAction;
	UFUNCTION()
	void OnTriggerAction(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, Category = "Collision")
	TEnumAsByte<ECollisionChannel> TraceChannelProperty = ECC_Pawn;
};
