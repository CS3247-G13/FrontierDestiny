// Fill out your copyright notice in the Description page of Project Settings.

#include "StraightProjectileActor.h"

AStraightProjectileActor::AStraightProjectileActor()
{
    // Default values
    Direction = FVector::ForwardVector;
    Speed = 2000.0f;
}

void AStraightProjectileActor::GetNextVelocityAndRotation_Implementation(float DeltaTime, FVector& OutVelocity, FRotator& OutRotation)
{
    // Calculate velocity based on direction and speed
    // We normalize the direction to ensure the speed is consistent
    FVector NormalizedDirection = Direction.GetSafeNormal();

    OutVelocity = NormalizedDirection * Speed;

    // Set rotation to face the direction of travel
    // If the direction is zero, we fall back to the current actor rotation
    if (!NormalizedDirection.IsNearlyZero())
    {
        OutRotation = NormalizedDirection.Rotation();
    }
    else
    {
        OutRotation = GetActorRotation();
    }
}

void AStraightProjectileActor::HitTarget_Implementation(const FHitResult& Hit)
{
    Super::HitTarget_Implementation(Hit);
    Destroy();
}