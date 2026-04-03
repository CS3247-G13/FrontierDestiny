// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileActor.h"
#include "CustomChannels.h"
#include "Components/PrimitiveComponent.h"

AProjectileActor::AProjectileActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AProjectileActor::BeginPlay()
{
    Super::BeginPlay();

    // Prevent immediate self-collision with the spawner/owner
    if (AActor* MyOwner = GetOwner())
    {
        if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent()))
        {
            RootPrim->IgnoreActorWhenMoving(MyOwner, true);
			// Set the bullet to not collide with other projectiles (Projectile channel)
            RootPrim->SetCollisionResponseToChannel(CC_Projectile, ECR_Ignore);
        }
    }
}

void AProjectileActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Process the movement and physics based on velocity and rotation
    Move(DeltaTime);
}

void AProjectileActor::Move(float DeltaTime)
{
    FVector Velocity = FVector::ZeroVector;
    FRotator Rotation = GetActorRotation();

    // 1. Get the current velocity and rotation from the child logic
    GetNextVelocityAndRotation(DeltaTime, Velocity, Rotation);

    // 2. If we aren't moving, we can still update rotation, but location remains same
    FVector CurrentLocation = GetActorLocation();
    FVector NewLocation = CurrentLocation + (Velocity * DeltaTime);

    // 3. Perform a combined Location and Rotation sweep
    FHitResult HitResult;
    SetActorLocationAndRotation(NewLocation, Rotation, true, &HitResult);

    // 4. Check for collision
    if (HitResult.bBlockingHit)
    {
        HitTarget(HitResult);
    }
}

void AProjectileActor::GetNextVelocityAndRotation_Implementation(float DeltaTime, FVector& OutVelocity, FRotator& OutRotation)
{
    // Default: Stationary and keep current rotation
    OutVelocity = FVector::ZeroVector;
    OutRotation = GetActorRotation();
}

void AProjectileActor::HitTarget_Implementation(const FHitResult& Hit)
{
    UE_LOG(LogTemp, Log, TEXT("Projectile hit %s at location: %s"),
        Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("None"),
        *Hit.ImpactPoint.ToString());
}