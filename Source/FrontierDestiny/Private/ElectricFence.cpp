// Fill out your copyright notice in the Description page of Project Settings.


#include "ElectricFence.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"


// Sets default values
AElectricFence::AElectricFence()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Initialize the mesh and set it as the root
	FenceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FenceMesh"));
	RootComponent = FenceMesh;

	// Initialize the collision box and attach it to the root
	DamageZone = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageZone"));
	DamageZone->SetupAttachment(RootComponent);
	
	// Set the collision profile so it actually triggers overlaps
	DamageZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

}

// Called when the game starts or when spawned
void AElectricFence::BeginPlay()
{
	Super::BeginPlay();
	if (DamageZone)
	{
		DamageZone->OnComponentBeginOverlap.AddDynamic(this, &AElectricFence::OnOverlapBegin);
	}
}

void AElectricFence::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if the overlapping actor is valid and isn't the fence itself
	if (OtherActor && (OtherActor != this))
	{
		// 1. Deal Damage (You can customize the DamageType class if needed)
		UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, nullptr, this, UDamageType::StaticClass());

		// 2. Spawn the Sparks
		if (SparkEffect)
		{
			// Try to spawn the sparks exactly where the collision happened. 
			// If it wasn't a sweep, fall back to the enemy's location.
			FVector SpawnLocation = SweepResult.IsValidBlockingHit() ? SweepResult.ImpactPoint : OtherActor->GetActorLocation();
			
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SparkEffect, SpawnLocation);
		}
	}
}

