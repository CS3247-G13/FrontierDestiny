// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"

#include "PlayerManagerSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "DamageNumber.h"
#include "EnemyManagerSubsystem.h"

#include "Engine/DamageEvents.h"

// For MassEntity damage
#include "MassEntityTypes.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassCommandBuffer.h"
#include "MassRepresentationSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EnemyDamageMassProcessor.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	/*WeaponDataMap.Add(EWeaponType::Rifle, FWeaponData{
		.Type = EWeaponType::Rifle,
		.Damage = 10,
		.FireRate = 3.33333f,
		.Spread = 5.f,
		.AmmoCost = 1
		});
	WeaponDataMap.Add(EWeaponType::Shotgun, FWeaponData{
		.Type = EWeaponType::Shotgun,
		.Damage = 10,
		.FireRate = 1.f,
		.Spread = 1.5f,
		.AmmoCost = 9
		});*/
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Cooldown= FMath::Max(0.f, Cooldown - DeltaTime);
	RecoverRecoil(DeltaTime);
}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetOwner()->GetWorldTimerManager().ClearTimer(ReplenishBulletTimerHandle);
}

void UCombatComponent::ActivateMode()
{
	Super::ActivateMode();
	OnCurrentBulletsChange.Broadcast(CurrentBullets, GetPlayerData().MaxBullets);
}

void UCombatComponent::SetupInput(UInputComponent* InputComponent)
{
	Super::SetupInput(InputComponent);

	// Cast the internal InputComponent to the Enhanced version
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PrimaryFireAction)
		{
			EnhancedInputComponent->BindAction(
				PrimaryFireAction,
				ETriggerEvent::Triggered,
				this,
				&UCombatComponent::OnFireAction,
				EWeaponType::Rifle
			);
			EnhancedInputComponent->BindAction(
				PrimaryFireAction,
				ETriggerEvent::Started,
				this,
				&UCombatComponent::OnFireActionStart,
				EWeaponType::Rifle
			);
		}
		if (SecondaryFireAction)
		{
			EnhancedInputComponent->BindAction(
				SecondaryFireAction,
				ETriggerEvent::Triggered,
				this,
				&UCombatComponent::OnFireAction,
				EWeaponType::Shotgun
			);
			EnhancedInputComponent->BindAction(
				SecondaryFireAction,
				ETriggerEvent::Started,
				this,
				&UCombatComponent::OnFireActionStart,
				EWeaponType::Shotgun
			);
		}
	}
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentBullets = GetPlayerData().MaxBullets;
	
	UpdateBulletReplenishTimer();

	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UPlayerManagerSubsystem* PM = GI->GetSubsystem<UPlayerManagerSubsystem>())
		{
			PM->OnPlayerStatsChanged.AddDynamic(this, &UCombatComponent::OnPlayerStatsUpdated);
		}
	}
}

void UCombatComponent::UpdateBulletReplenishTimer()
{
	GetOwner()->GetWorldTimerManager().ClearTimer(ReplenishBulletTimerHandle);
	GetOwner()->GetWorldTimerManager().SetTimer(
		ReplenishBulletTimerHandle,
		this,
		&UCombatComponent::ReplenishBullet,
		1.f / GetPlayerData().GetAmmoReplenishRate(),
		true
	);
}

void UCombatComponent::ReplenishBullet()
{
	CurrentBullets = FMath::Min(CurrentBullets + 1, GetPlayerData().MaxBullets);
	OnCurrentBulletsChange.Broadcast(CurrentBullets, GetPlayerData().MaxBullets);
}

void UCombatComponent::OnFireActionStart(EWeaponType WeaponType)
{
	if (WeaponType != CurrentWeapon)
	{
		OnWeaponChange.Broadcast(WeaponType);
		CurrentWeapon = WeaponType;
	}

	if (CurrentBullets < GetPlayerData().WeaponDataMap[CurrentWeapon].AmmoCost)
	{
		if (GetPlayerData().WeaponDataMap[CurrentWeapon].OutOfBulletsSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), GetPlayerData().WeaponDataMap[CurrentWeapon].OutOfBulletsSound);
		}
	}
}

void UCombatComponent::OnFireAction(EWeaponType WeaponType)
{
	// Check if can fire
	if (Cooldown > SMALL_NUMBER)
	{
		return;
	}
	if (!TryConsumeBullets())
	{
		return;
	}

	Shoot();
	PlayTriggerSound();
	Recoil();
	PutOnCooldown();
}

void UCombatComponent::Shoot()
{
	float YawRad, PitchRad;
	FVector Direction, BulletDirection;

	Direction = Camera->GetForwardVector();

	switch (CurrentWeapon)
	{
	case EWeaponType::Rifle:
		YawRad = FMath::DegreesToRadians(CurrentOffset.X);
		
		BulletDirection = Direction + Camera->GetRightVector() * YawRad;
		BulletDirection.Normalize();
		ShootDirection(BulletDirection);
		break;
	case EWeaponType::Shotgun:
		Direction = Camera->GetForwardVector();
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				// Convert angles to Radians for math functions
				YawRad = FMath::DegreesToRadians(i * GetPlayerData().WeaponDataMap[CurrentWeapon].GetSpread());
				PitchRad = FMath::DegreesToRadians(j * GetPlayerData().WeaponDataMap[CurrentWeapon].GetSpread());

				// Build the direction by adding offsets to the forward vector
				// This stays consistent regardless of world orientation
				BulletDirection = Direction
					+ (Camera->GetRightVector() * YawRad)
					+ (Camera->GetUpVector() * PitchRad);

				BulletDirection.Normalize();
				ShootDirection(BulletDirection);
			}
		}
		break;
	}
}

void UCombatComponent::ShootDirection(FVector Direction)
{
	FHitResult Hit;
	FVector TraceStart = Camera->GetComponentLocation();
	FVector TraceEnd = TraceStart + Direction * GetPlayerData().WeaponDataMap[CurrentWeapon].Range;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Pawn);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ShootingTargetChannel, QueryParams);

	if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
	{
		FTransform SpawnTransform((-Direction).Rotation(), TraceStart + Direction * 100.f);
		UGameplayStatics::ApplyDamage(Hit.GetActor(), GetPlayerData().WeaponDataMap[CurrentWeapon].GetDamage(), PlayerController, Pawn, UDamageType::StaticClass());
	}

	UE_LOG(LogTemp, Warning, TEXT("HIT"));
	// Try to hit MassEntity
	{
		// Cast the hit component to ISMC
		UInstancedStaticMeshComponent* HitISMC = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());

		UEnemyManagerSubsystem* EnemyManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();

		FMassEntityHandle Handle = EnemyManagerSubsystem->GetEnemyEntityHandle(HitISMC, Hit.Item);

		EnemyManagerSubsystem->ApplyDamageToEnemy(Handle, GetPlayerData().WeaponDataMap[CurrentWeapon].GetDamage());
	}
}

bool UCombatComponent::TryConsumeBullets()
{
	if (CurrentBullets < GetPlayerData().WeaponDataMap[CurrentWeapon].AmmoCost)
	{
		return false;
	}
	CurrentBullets = FMath::Max(0, CurrentBullets - GetPlayerData().WeaponDataMap[CurrentWeapon].AmmoCost);
	OnCurrentBulletsChange.Broadcast(CurrentBullets, GetPlayerData().MaxBullets);
	return true;
}

void UCombatComponent::PlayTriggerSound()
{
	if (GetPlayerData().WeaponDataMap[CurrentWeapon].TriggerSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), GetPlayerData().WeaponDataMap[CurrentWeapon].TriggerSound);
	}
}

void UCombatComponent::Recoil()
{
	FWeaponData Data = GetPlayerData().WeaponDataMap[CurrentWeapon];
	int Sign = FMath::RandBool() ? 1 : -1;
	FVector2D NextOffset = CurrentOffset + FVector2D(Data.RecoilAmount.X * Sign, Data.RecoilAmount.Y);
	NextOffset.X = FMath::Min(NextOffset.X, Data.MaxRecoilAmount.X);
	NextOffset.Y = FMath::Min(NextOffset.Y, Data.MaxRecoilAmount.Y);
	FVector2D DeltaOffset = NextOffset - CurrentOffset;
	Pawn->AddControllerPitchInput(-DeltaOffset.Y);
	CurrentOffset = NextOffset;
}

void UCombatComponent::PutOnCooldown()
{
	Cooldown = 1.f / GetPlayerData().WeaponDataMap[CurrentWeapon].GetFireRate();
}

void UCombatComponent::RecoverRecoil(float DeltaSeconds)
{
	FWeaponData Data = GetPlayerData().WeaponDataMap[CurrentWeapon];
	float DeltaOffsetX = FMath::Min(CurrentOffset.X, Data.OffsetRecoverySpeed.X * DeltaSeconds);
	float DeltaOffsetY = FMath::Min(CurrentOffset.Y, Data.OffsetRecoverySpeed.Y * DeltaSeconds);
	CurrentOffset -= FVector2D(DeltaOffsetX, DeltaOffsetY);
	Pawn->AddControllerPitchInput(DeltaOffsetY);
}

void UCombatComponent::OnPlayerStatsUpdated()
{
	UpdateBulletReplenishTimer();
}