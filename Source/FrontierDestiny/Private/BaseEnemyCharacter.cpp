// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyManagerSubsystem.h"
#include "BaseEnemyCharacter.h"
#include "QuestSubsystem.h"
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
	
	StatComponent->Attributes = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>()->GetEnemyData(EnemyID).Attributes;
	StatComponent->InitializeStats();

	for (UEnemyModifier* Mod : StartingModifiers)
	{
		StatComponent->ApplyModifier(Mod);
	}
	float speed = StatComponent->GetStat("Speed");
	// GetCharacterMovement()->MaxWalkSpeed = StatComponent->GetStat(TEXT("Speed"));
	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Current Speed: %f"), speed));
}

float ABaseEnemyCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// apply modifiers
	// prob some damage reduction thing
	float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	StatComponent->AddValue(TEXT("HP"), -Damage);
	if (StatComponent->GetStat(TEXT("HP")) <= 0.f)
	{
		UQuestSubsystem* QuestSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UQuestSubsystem>();
		if (IsValid(QuestSubsystem))
		{
			QuestSubsystem->RegisterEnemyKilled();
		}
		OnDeath.Broadcast();
		Destroy();
	}

	return Damage;
}