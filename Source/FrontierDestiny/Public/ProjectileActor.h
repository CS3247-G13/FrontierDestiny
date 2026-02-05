// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileActor.generated.h"

UCLASS()
class FRONTIERDESTINY_API AProjectileActor : public AActor
{
    GENERATED_BODY()

public:
    AProjectileActor();

protected:
    virtual void BeginPlay() override;

    /** * The internal movement logic that handles the physical move, rotation, and sweeping.
     * This is NOT intended to be overridden by children.
     */
    void Move(float DeltaTime);

    /** * Hook for children to define the velocity and orientation of the projectile.
     * @param DeltaTime Time since last frame.
     * @param OutVelocity [Out] The calculated velocity vector for this frame.
     * @param OutRotation [Out] The calculated rotation for the projectile.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Projectile | Movement")
    void GetNextVelocityAndRotation(float DeltaTime, FVector& OutVelocity, FRotator& OutRotation);
    virtual void GetNextVelocityAndRotation_Implementation(float DeltaTime, FVector& OutVelocity, FRotator& OutRotation);

    /** * Triggered when the Sweep in Move() hits a blocking object.
     * @param TargetActor The actor that was hit.
     * @param HitLocation The specific coordinates of the impact point.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Projectile | Combat")
    void HitTarget(AActor* TargetActor, FVector HitLocation);
    virtual void HitTarget_Implementation(AActor* TargetActor, FVector HitLocation);

public:
    virtual void Tick(float DeltaTime) override;
};
