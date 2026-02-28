// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"

#include "Camera/CameraComponent.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"

#include "Engine/DamageEvents.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UCombatComponent::SetupInput(UInputComponent* InputComponent)
{
	Super::SetupInput(InputComponent);

	// Cast the internal InputComponent to the Enhanced version
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (TriggerAction)
		{
			EnhancedInputComponent->BindAction(
				TriggerAction,
				ETriggerEvent::Started,
				this,
				&UCombatComponent::OnTriggerAction
			);
		}
	}
}

void UCombatComponent::OnTriggerAction(const FInputActionValue& Value)
{
	FHitResult Hit;
	FVector TraceStart = Camera->GetComponentLocation();
	FVector Direction = Camera->GetForwardVector();
	FVector TraceEnd = TraceStart + Direction * 1000.0f;	//TODO replace magic number
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Pawn);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannelProperty, QueryParams);
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, Hit.bBlockingHit ? FColor::Blue : FColor::Red, false, 5.0f, 0, 10.0f);

	if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
	{
		FString hit = *Hit.GetActor()->GetName();
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("hit ") + hit);

		// Damage enemy
		FPointDamageEvent DamageEvent;
		DamageEvent.HitInfo = Hit;
		DamageEvent.ShotDirection = Direction;
		DamageEvent.DamageTypeClass = UDamageType::StaticClass();
		Hit.GetActor()->TakeDamage(10, DamageEvent, Pawn->GetController(), GetOwner());
	}
}
