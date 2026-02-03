// Fill out your copyright notice in the Description page of Project Settings.


#include "SingleTargetTowerActor.h"
#include "EnemyActor.h"
#include "Kismet/GameplayStatics.h"

ASingleTargetTowerActor::ASingleTargetTowerActor()
{

}

bool ASingleTargetTowerActor::SelectTarget()
{
	Super::SelectTarget();

	AEnemyActor* ClosestEnemy = nullptr;
	float ClosestDistance = TNumericLimits<float>::Max();

	for (AEnemyActor* enemy : PotentialTargets)
	{
		if (enemy && enemy->CurrentHealth > 0)
		{
			float Distance = 
				FVector::Distance(
					enemy->GetActorTransform().GetLocation(),
					this->GetActorTransform().GetLocation());
			
			if (Distance < ClosestDistance) {
				ClosestDistance = Distance;
				ClosestEnemy = enemy;
			}
		}
	}

	if (ClosestEnemy != nullptr) {
		if (Target == ClosestEnemy) {
			return true;
		}
		if (Target != nullptr) {
			Target->OnEnemyDied.RemoveDynamic(this, &ASingleTargetTowerActor::OnTargetDeath);
		}
		Target = ClosestEnemy;
		Target->OnEnemyDied.AddDynamic(this, &ASingleTargetTowerActor::OnTargetDeath);
		return true;
	}
	return false;
}

void ASingleTargetTowerActor::OnTargetDeath() {
	PotentialTargets.Remove(Target);
	Target = nullptr;
	SelectTarget();
}

bool ASingleTargetTowerActor::AttackTarget()
{
	if (Target == nullptr) {
		return false;
	}
	float DamageToDeal = 10.0f;
	UGameplayStatics::ApplyDamage(Target, DamageToDeal, PlayerController, this, nullptr);

	return true;
}
