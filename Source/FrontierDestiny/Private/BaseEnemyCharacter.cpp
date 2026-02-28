// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseEnemyCharacter.h"
#include "StatComponent.h"
#include "EnemyModifier.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ABaseEnemyCharacter::ABaseEnemyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));

	// have beginplay for the component
	//StatComponent->bAutoActivate = true;

}

// Called when the game starts or when spawned
void ABaseEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	for (UEnemyModifier* Mod : StartingModifiers)
	{
		StatComponent->ApplyModifier(Mod);
	}
	float speed = StatComponent->GetStat("Speed");
	//GetCharacterMovement()->MaxWalkSpeed = StatComponent->GetStat(TEXT("Speed"));
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Current Speed: %f"), speed));
}

// Called every frame
void ABaseEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABaseEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float ABaseEnemyCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// apply modifiers
	// prob some damage reduction thing
	StatComponent->AddValue(TEXT("HP"), -DamageAmount);
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}