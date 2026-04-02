// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"
#include "Animation/AnimationAsset.h"
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
	OnCurrentBulletsChange.Broadcast(CurrentBullets, GetPlayerData().GetMaxBullets());
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

	CurrentBullets = GetPlayerData().GetMaxBullets();
	
	UpdateBulletReplenishTimer();

	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UPlayerManagerSubsystem* PM = GI->GetSubsystem<UPlayerManagerSubsystem>())
		{
			PM->OnPlayerStatsChanged.AddDynamic(this, &UCombatComponent::OnPlayerStatsUpdated);
			PM->OnRestoreBullets.AddDynamic(this, &UCombatComponent::OnRestoreBullets);
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
	CurrentBullets = FMath::Min(CurrentBullets + 1, GetPlayerData().GetMaxBullets());
	OnCurrentBulletsChange.Broadcast(CurrentBullets, GetPlayerData().GetMaxBullets());
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

	const FPlayerData& D = GetPlayerData();

	Shoot();
	PlayFireAnimation(WeaponType);
	PlayTriggerSound();
	Recoil();
	PutOnCooldown();
}

void UCombatComponent::PlayFireAnimation(EWeaponType WeaponType)
{
	if (MuzzleFlashShotgun && CurrentWeapon == EWeaponType::Shotgun)
		MuzzleFlashShotgun->Activate(true);

	if (MuzzleFlashSniper && CurrentWeapon == EWeaponType::Rifle)
		MuzzleFlashSniper->Activate(true);
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
	{
		Direction = Camera->GetForwardVector();
		if (GetPlayerData().bSlugConversion)
		{
			ShootDirection(Direction);
			break;
		}
		const bool bFlak = GetPlayerData().bFlakBarrel;
		const TArray<float> HOffsets = bFlak ? TArray<float>{ -3, -2, -1, 0, 1, 2, 3 } : TArray<float>{ -1, 0, 1 };
		const TArray<float> VOffsets = bFlak ? TArray<float>{ -0.5f, 0.5f }            : TArray<float>{ -1, 0, 1 };

		for (float H : HOffsets)
		{
			for (float V : VOffsets)
			{
				YawRad   = FMath::DegreesToRadians(H * GetPlayerData().WeaponDataMap[CurrentWeapon].GetSpread());
				PitchRad = FMath::DegreesToRadians(V * GetPlayerData().WeaponDataMap[CurrentWeapon].GetSpread());

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
}

void UCombatComponent::ShootDirection(FVector Direction)
{
	FVector TraceStart = Camera->GetComponentLocation();
	FVector TraceEnd = TraceStart + Direction * GetPlayerData().WeaponDataMap[CurrentWeapon].Range;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Pawn);

	if (CurrentWeapon == EWeaponType::Rifle && GetPlayerData().bPiercingShots)
	{
		TArray<FHitResult> Hits;
		GetWorld()->LineTraceMultiByChannel(Hits, TraceStart, TraceEnd, ECC_GameTraceChannel3, QueryParams);
		UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
		TSet<FMassEntityHandle> HitHandles;
		for (const FHitResult& Hit : Hits)
		{
			FMassEnemyTarget Target;
			if (EnemyManager->GetEnemyTargetFromHit(Hit, Target))
			{
				if (HitHandles.Contains(Target.EntityHandle))
				{
					continue;
				}
				HitHandles.Add(Target.EntityHandle);
			}
			ApplyHit(Hit);
		}
	}
	else
	{
		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ShootingTargetChannel, QueryParams);
		ApplyHit(Hit);
	}
}

void UCombatComponent::ApplyHit(const FHitResult& Hit)
{
	UEnemyManagerSubsystem* EnemyManager = GetWorld()->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>();
	const int32 Damage = (CurrentWeapon == EWeaponType::Shotgun && GetPlayerData().bSlugConversion)
		? static_cast<int32>(GetPlayerData().SlugConversionDamage)
		: GetPlayerData().WeaponDataMap[CurrentWeapon].GetDamage();
	EnemyManager->ApplyDamageByHit(Hit, Damage, EDamageType::Kinetic);

	FMassEnemyTarget Target;
	if (EnemyManager->GetEnemyTargetFromHit(Hit, Target))
	{
		switch (CurrentWeapon)
		{
		case EWeaponType::Rifle:   ApplyRifleUpgrades(EnemyManager, Target);        break;
		case EWeaponType::Shotgun: ApplyShotgunUpgrades(EnemyManager, Target, Hit); break;
		}
	}
}

void UCombatComponent::ApplyRifleUpgrades(UEnemyManagerSubsystem* EnemyManager, FMassEnemyTarget Target)
{
	const FPlayerData& Data = GetPlayerData();

	if (Data.bRuptureRounds)
		EnemyManager->ApplyRuptured(Target, Data.RuptureRoundsDamage, Data.RuptureRoundsRadius);

	if (Data.bSuppressingFire)
		EnemyManager->ApplySuppressed(Target, Data.SuppressingFireShotCount, Data.SuppressingFireTimeWindow,
			Data.SuppressingFireSlowAmount, Data.SuppressingFireSlowDuration);

	if (Data.bCompoundingInjury)
		EnemyManager->ApplyCompoundingInjury(Target, Data.CompoundingInjuryDamagePerStack,
			Data.CompoundingInjuryMaxBonus, Data.CompoundingInjuryResetTime);

	if (Data.bArcShots)
		EnemyManager->ApplyArcShot(Target, Data.ArcShotRange, Data.ArcShotDamage);

	if (Data.bConduitMarker)
		EnemyManager->ApplyConduitMarker(Target, Data.ConduitMarkerDuration,
			Data.ConduitMarkerRange, Data.ConduitMarkerArcDamage);
}

void UCombatComponent::ApplyShotgunUpgrades(UEnemyManagerSubsystem* EnemyManager, FMassEnemyTarget Target, const FHitResult& Hit)
{
	const FPlayerData& Data = GetPlayerData();

	if (Data.bInfernoCartridge)
		EnemyManager->ApplyBurn(Target, Data.InfernoCartridgeDuration,
			Data.InfernoCartridgeDamagePerTick, Data.InfernoCartridgeTickInterval);

	if (Data.bStaggerShells && Hit.Distance <= Data.StaggerShellsRange)
		EnemyManager->ApplyStun(Target, Data.StaggerShellsStunDuration);

	if (Data.bBallisticRecall && Hit.Distance <= Data.BallisticRecallRange)
		EnemyManager->ApplyBallisticRecall(Target, Data.BallisticRecallAmmoRegain);

	if (Data.bDevastatingBlow)
		EnemyManager->ApplyDevastatingBlow(Target, Data.DevastatingBlowHPThreshold, Data.DevastatingBlowMultiplier);
}

bool UCombatComponent::TryConsumeBullets()
{
	if (CurrentBullets < GetPlayerData().WeaponDataMap[CurrentWeapon].AmmoCost)
	{
		return false;
	}
	CurrentBullets = FMath::Max(0, CurrentBullets - GetPlayerData().WeaponDataMap[CurrentWeapon].AmmoCost);
	OnCurrentBulletsChange.Broadcast(CurrentBullets, GetPlayerData().GetMaxBullets());
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

void UCombatComponent::OnRestoreBullets(int32 Amount)
{
	CurrentBullets = FMath::Min(CurrentBullets + Amount, GetPlayerData().GetMaxBullets());
	OnCurrentBulletsChange.Broadcast(CurrentBullets, GetPlayerData().GetMaxBullets());
}