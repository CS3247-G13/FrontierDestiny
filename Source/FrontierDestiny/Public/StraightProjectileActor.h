// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectileActor.h"
#include "StraightProjectileActor.generated.h"

/**
 * A projectile that moves in a straight line based on a direction vector and speed.
 */
UCLASS()
class FRONTIERDESTINY_API AStraightProjectileActor : public AProjectileActor
{
    GENERATED_BODY()

public:
    AStraightProjectileActor();

    /** The direction the projectile is moving. Set at spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Projectile")
    FVector Direction;

    /** The speed at which the projectile travels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Projectile")
    float Speed;

protected:
    /** * Provides the constant velocity and orientation based on the Direction vector.
     */
    virtual void GetNextVelocityAndRotation_Implementation(float DeltaTime, FVector& OutVelocity, FRotator& OutRotation) override;

    /** Handles logic when the projectile hits an object */
    virtual void HitTarget_Implementation(AActor* TargetActor, FVector HitLocation) override;
};