// Fill out your copyright notice in the Description page of Project Settings.


#include "StatComponent.h"
#include "EnemyModifier.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values for this component's properties
UStatComponent::UStatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	// PrimaryComponentTick.bCanEverTick = true;

	// ...
	Attributes.Add(TEXT("HP"), FEntityAttribute{ 100.0f, 100.0f, 100.0f });
	Attributes.Add(TEXT("Speed"), FEntityAttribute{ 600.0f, 600.0f, 600.0f });
	Attributes.Add(TEXT("DMG"), FEntityAttribute{ 10.0f, 10.0f, 10.0f });
}


// Called when the game starts
void UStatComponent::BeginPlay()
{
	Super::BeginPlay();


}


// Called every frame
void UStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UStatComponent::ApplyModifier(UEnemyModifier* Modifier)
{
	if (!Modifier) return;

	ActiveModifiers.Add(Modifier);
	RecalculateStat(Modifier->TargetStatName);
}

void UStatComponent::RecalculateStat(FName StatName)
{
	if (!Attributes.Contains(StatName)) return;

	float NewValue = Attributes[StatName].BaseValue;
	for (UEnemyModifier* Mod : ActiveModifiers)
	{
		if (Mod->TargetStatName == StatName)
		{
			NewValue *= Mod->Multiplier;
			NewValue += Mod->Adder;
		}
	}
	Attributes[StatName].CurrentMaxValue = NewValue;
	Attributes[StatName].CurrentValue = FMath::Clamp(Attributes[StatName].CurrentValue, 0.0f, Attributes[StatName].CurrentMaxValue);
	if (StatName == TEXT("Speed")) {
		if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
		{
			Owner->GetCharacterMovement()->MaxWalkSpeed = NewValue;
		}
	}
}

void UStatComponent::AddValue(FName StatName, float Amount)
{
	if (!Attributes.Contains(StatName)) return;
	Attributes[StatName].CurrentValue = FMath::Clamp(Attributes[StatName].CurrentValue + Amount, 0.0f, Attributes[StatName].CurrentMaxValue);
}
