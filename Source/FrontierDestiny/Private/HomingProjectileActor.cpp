// Fill out your copyright notice in the Description page of Project Settings.


#include "HomingProjectileActor.h"
#include "Kismet/KismetMathLibrary.h"

AHomingProjectileActor::AHomingProjectileActor()
{
    // Default values
    StartingVelocity = FVector::ZeroVector;
    RotationalSpeed = 200.0f;
    ClearanceDuration = 0.2f;
    ClearanceSecondsRemaining = ClearanceDuration;
    LastTargetLocation = FVector::ZeroVector;
}

void AHomingProjectileActor::BeginPlay()
{
    Super::BeginPlay();

    CurrentVelocity = StartingVelocity;

    if (IsValid(Target))
    {
        LastTargetLocation = Target->GetComponentLocation();
    }
    else
    {
        LastTargetLocation = GetActorLocation();
    }

    ClearanceSecondsRemaining = ClearanceDuration;
}

void AHomingProjectileActor::GetNextVelocityAndRotation_Implementation(float DeltaTime, FVector& OutVelocity, FRotator& OutRotation)
{
    // 1. Process Clearance Phase
    if (ClearanceSecondsRemaining > 0.0f)
    {
        ClearanceSecondsRemaining -= DeltaTime;

        // Maintain straight movement
        OutVelocity = CurrentVelocity;
        OutRotation = CurrentVelocity.Rotation();
    }
    // 2. Homing Phase
    else
    {
        FVector TargetCurrentLocation;
        if (IsValid(Target))
        {
            TargetCurrentLocation = Target->GetComponentLocation();
        }
        else
        {
            TargetCurrentLocation = LastTargetLocation;
        }

        // Calculate Target Velocity (how fast and where the target is moving)
        // We use DeltaTime to get units per second
        FVector TargetVelocity = FVector::ZeroVector;
        if (DeltaTime > 0.0f)
        {
            TargetVelocity = (TargetCurrentLocation - LastTargetLocation) / DeltaTime;
        }

        // --- PREDICTIVE INTERCEPT MATH ---
        FVector CurrentLocation = GetActorLocation();
        float DistanceToTarget = FVector::Dist(CurrentLocation, TargetCurrentLocation);
        float RocketSpeed = CurrentVelocity.Size();

        // Calculate how long it will take to reach the target's CURRENT position
        // We use a small epsilon check to avoid division by zero
        float TimeToReach = (RocketSpeed > 1.0f) ? (DistanceToTarget / RocketSpeed) : 0.0f;

        // Predict where the target will be when we arrive
        FVector PredictedTargetLocation = TargetCurrentLocation + (TargetVelocity * TimeToReach);

        // Steering logic: Rotate CurrentVelocity toward the PredictedTargetLocation
        FVector DesiredDirection = (PredictedTargetLocation - CurrentLocation).GetSafeNormal();

        FRotator CurrentRot = CurrentVelocity.Rotation();
        FRotator TargetRot = DesiredDirection.Rotation();

        // Smoothly interpolate rotation based on RotationalSpeed
        FRotator SteeredRot = UKismetMathLibrary::RInterpTo_Constant(CurrentRot, TargetRot, DeltaTime, RotationalSpeed);

        // Update the persistent velocity (maintaining the original speed)
        CurrentVelocity = SteeredRot.Vector() * RocketSpeed;

        OutVelocity = CurrentVelocity;
        OutRotation = SteeredRot;
    }

    // Update Last Target Location for next frame's prediction
    if (IsValid(Target))
    {
        LastTargetLocation = Target->GetComponentLocation();
    }

    return;
}

void AHomingProjectileActor::HitTarget_Implementation(AActor* TargetActor, FVector HitLocation)
{
    Super::HitTarget_Implementation(TargetActor, HitLocation); 
    Destroy();
}