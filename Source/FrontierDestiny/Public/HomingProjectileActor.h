// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectileActor.h"
#include "HomingProjectileActor.generated.h"

/**
 * A projectile that tracks a target actor using rotational steering and
 * predictive calculations.
 */
UCLASS()
class FRONTIERDESTINY_API AHomingProjectileActor : public AProjectileActor
{
    GENERATED_BODY()

public:
    AHomingProjectileActor();

    /** The initial velocity vector. Set at spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Projectile")
    FVector StartingVelocity;

    /** The actor we are trying to hit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Projectile")
    TObjectPtr<USceneComponent> Target;

    /** How fast the projectile can turn toward the target (degrees per second) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Projectile")
    float RotationalSpeed;

    /** Time in seconds the projectile moves straight before homing begins. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float ClearanceDuration;

protected:
    virtual void BeginPlay() override;

    /** * Calculates the steering velocity and orientation toward the target.
     */
    virtual void GetNextVelocityAndRotation_Implementation(float DeltaTime, FVector& OutVelocity, FRotator& OutRotation) override;

    /** Handles impact logic */
    virtual void HitTarget_Implementation(AActor* TargetActor, FVector HitLocation) override;

private:
    /** The current velocity of the projectile, updated frame by frame */
    FVector CurrentVelocity;

    /** Used to calculate the target's current velocity for predictive aiming. */
    FVector LastTargetLocation;

    /** Countdown timer for the initial straight-line clearance phase. */
    float ClearanceSecondsRemaining;
};