// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"

#include "UpgradeSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "DamageNumber.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	WeaponDataMap.Add(EWeaponType::Rifle, FWeaponData{
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
		});

	WeaponCooldowns.Add(EWeaponType::Rifle, 0.f);
	WeaponCooldowns.Add(EWeaponType::Shotgun, 0.f);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	for (auto& Pair : WeaponCooldowns)
	{
		if (Pair.Value > SMALL_NUMBER)
		{
			Pair.Value = FMath::Max(0.f, Pair.Value - DeltaTime);
		}
	}

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
	OnCurrentBulletsChange.Broadcast(CurrentBullets, MaxBullets);
}

void UCombatComponent::OnUpgraded(const FUpgradeData& Upgrade)
{
	if (Upgrade.TargetID == "rifle" || Upgrade.TargetID == "shotgun")
	{
		FWeaponData& Data = WeaponDataMap[
			Upgrade.TargetID == "rifle"
				? EWeaponType::Rifle
				: Upgrade.TargetID == "shotgun"
				? EWeaponType::Shotgun
				: EWeaponType::Rifle];

		static const TArray<EUpgradeProperty> WeaponStatProperties = {
			EUpgradeProperty::WeaponDamageAdded,
			EUpgradeProperty::WeaponDamageMultiplier,
			EUpgradeProperty::WeaponFireRateAdded,
			EUpgradeProperty::WeaponFireRateMultiplier,
			EUpgradeProperty::WeaponSpreadReduction,
		};
		
		// Copy it so we can manipulate it to add defaults
		TMap<EUpgradeProperty, float> Properties = Upgrade.Properties;
		for (EUpgradeProperty Property : WeaponStatProperties)
		{
			if (!Properties.Contains(Property))
			{
				Properties.Add(Property, 0.f);
			}
		}
		

		// Apply the stat changes
		Data.DamageAdded += Properties[EUpgradeProperty::WeaponDamageAdded];
		Data.DamageMultiplier += Properties[EUpgradeProperty::WeaponDamageMultiplier];
		Data.FireRateAdded += Properties[EUpgradeProperty::WeaponFireRateAdded];
		Data.FireRateMultiplier += Properties[EUpgradeProperty::WeaponFireRateMultiplier];
		Data.SpreadReduction += Properties[EUpgradeProperty::WeaponSpreadReduction];

		return;
	}

	else if (Upgrade.TargetID == "player")
	{
		// Handle the player upgrade
		static const TArray<EUpgradeProperty> PlayerStatProperties = {
			EUpgradeProperty::PlayerAmmoReplenishRateAdded,
			EUpgradeProperty::PlayerAmmoReplenishRateMultiplier
		};
		TMap<EUpgradeProperty, float> Properties = Upgrade.Properties;
		for (EUpgradeProperty Property : PlayerStatProperties)
		{
			if (!Properties.Contains(Property))
			{
				Properties.Add(Property, 0.f);
			}
		}
		
		AmmoReplenishRateAdded += Properties[EUpgradeProperty::PlayerAmmoReplenishRateAdded];
		AmmoReplenishRateMultiplier += Properties[EUpgradeProperty::PlayerAmmoReplenishRateMultiplier];

		// The ammo replenish speed may have changed
		UpdateBulletReplenishTimer();
	}
}

void UCombatComponent::SetupInput(UInputComponent* InputComponent)
{
	Super::SetupInput(InputComponent);

	// Cast the internal InputComponent to the Enhanced version
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (TriggerAction)
		{
			EnhancedInputComponent->BindAction(
				TriggerAction,
				ETriggerEvent::Triggered,
				this,
				&UCombatComponent::OnTriggerAction
			);
			EnhancedInputComponent->BindAction(
				TriggerAction,
				ETriggerEvent::Started,
				this,
				&UCombatComponent::OnTriggerStartAction
			);
		}
		if (SwapWeaponAction)
		{
			EnhancedInputComponent->BindAction(
				SwapWeaponAction,
				ETriggerEvent::Triggered,
				this,
				&UCombatComponent::OnSwapWeaponAction
			);
		}
		if (SelectWeaponAction)
		{
			EnhancedInputComponent->BindAction(
				SelectWeaponAction,
				ETriggerEvent::Triggered,
				this,
				&UCombatComponent::OnSelectWeaponAction
			);
		}
	}
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentBullets = MaxBullets;
	
	UpdateBulletReplenishTimer();

	UUpgradeManagerSubsystem* UpgradeManager = GetWorld()->GetGameInstance()->GetSubsystem<UUpgradeManagerSubsystem>();
	UpgradeManager->OnUpgradePerformed.AddDynamic(this, &UCombatComponent::OnUpgraded);
}

void UCombatComponent::UpdateBulletReplenishTimer()
{
	GetOwner()->GetWorldTimerManager().ClearTimer(ReplenishBulletTimerHandle);
	GetOwner()->GetWorldTimerManager().SetTimer(
		ReplenishBulletTimerHandle,
		this,
		&UCombatComponent::ReplenishBullet,
		1.f / GetAmmoReplenishRate(),
		true
	);
}

void UCombatComponent::ReplenishBullet()
{
	CurrentBullets = FMath::Min(CurrentBullets + 1, MaxBullets);
	OnCurrentBulletsChange.Broadcast(CurrentBullets, MaxBullets);
}

void UCombatComponent::OnTriggerStartAction(const FInputActionValue& Value)
{
	if (CurrentBullets < WeaponDataMap[CurrentWeapon].AmmoCost)
	{
		if (WeaponDataMap[CurrentWeapon].OutOfBulletsSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), WeaponDataMap[CurrentWeapon].OutOfBulletsSound);
		}
	}
}

void UCombatComponent::OnSwapWeaponAction(const FInputActionValue& Value)
{
	SwapWeapon();
}

void UCombatComponent::OnSelectWeaponAction(const FInputActionValue& Value)
{
	int32 InputValue = FMath::RoundToInt32(Value.Get<float>());
	uint8 WeaponIndex = static_cast<uint8>(FMath::Max(0, (InputValue - 1) % 2));
	SelectWeapon(static_cast<EWeaponType>(WeaponIndex));
}

void UCombatComponent::SwapWeapon()
{
	SelectWeapon(static_cast<EWeaponType>((static_cast<uint8>(CurrentWeapon) + 1) % 2));
}

void UCombatComponent::SelectWeapon(EWeaponType WeaponType)
{
	// Supposed to play an animation and use a callback to reenable the shooting but here we are
	CurrentWeapon = WeaponType;

	// 1. Get the Enum as a UEnum pointer
	const UEnum* EnumPtr = StaticEnum<EWeaponType>();

	// 2. Convert the current value to a FString, then to TCHAR* for the log
	FString WeaponName = EnumPtr->GetValueAsString(CurrentWeapon);
	OnWeaponChange.Broadcast(CurrentWeapon);

	UE_LOG(LogTemp, Display, TEXT("Changed weapon to %s"), *WeaponName);
}

void UCombatComponent::OnTriggerAction(const FInputActionValue& Value)
{
	// Check if can fire
	if (WeaponCooldowns[CurrentWeapon] > SMALL_NUMBER)
	{
		return;
	}

	Shoot();
	ConsumeBullets();
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
				YawRad = FMath::DegreesToRadians(i * WeaponDataMap[CurrentWeapon].GetSpread());
				PitchRad = FMath::DegreesToRadians(j * WeaponDataMap[CurrentWeapon].GetSpread());

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
	FVector TraceEnd = TraceStart + Direction * WeaponDataMap[CurrentWeapon].Range;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Pawn);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ShootingTargetChannel, QueryParams);

	if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
	{
		FTransform SpawnTransform((-Direction).Rotation(), TraceStart + Direction * 100.f);
		DrawDebugBox(GetWorld(), Hit.Location, FVector(5.f, 5.f, 5.f), FColor::Red, true, 1.f, 0, 1.f);
		DrawDebugLine(GetWorld(), TraceStart, Hit.Location, FColor::Green, true, 1.f, 0, 0.1f);
	}
}

void UCombatComponent::ConsumeBullets()
{
	CurrentBullets = FMath::Max(0, CurrentBullets - WeaponDataMap[CurrentWeapon].AmmoCost);
	OnCurrentBulletsChange.Broadcast(CurrentBullets, MaxBullets);
}

void UCombatComponent::PlayTriggerSound()
{
	if (WeaponDataMap[CurrentWeapon].TriggerSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), WeaponDataMap[CurrentWeapon].TriggerSound);
	}
}

void UCombatComponent::Recoil()
{
	FWeaponData Data = WeaponDataMap[CurrentWeapon];
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
	WeaponCooldowns[CurrentWeapon] = 1.f / WeaponDataMap[CurrentWeapon].GetFireRate();
}

void UCombatComponent::RecoverRecoil(float DeltaSeconds)
{
	FWeaponData Data = WeaponDataMap[CurrentWeapon];
	float DeltaOffsetX = FMath::Min(CurrentOffset.X, Data.OffsetRecoverySpeed.X * DeltaSeconds);
	float DeltaOffsetY = FMath::Min(CurrentOffset.Y, Data.OffsetRecoverySpeed.Y * DeltaSeconds);
	CurrentOffset -= FVector2D(DeltaOffsetX, DeltaOffsetY);
	Pawn->AddControllerPitchInput(DeltaOffsetY);
}