// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"

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
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	CurrentCooldown = FMath::Max(0.f, CurrentCooldown - DeltaTime);

	switch (CurrentWeapon)
	{
	case EWeaponType::Rifle:
		RifleRecoverRecoil(DeltaTime);
		break;
	case EWeaponType::Shotgun:
		ShotgunRecoverRecoil(DeltaTime);
		break;
	}
}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetOwner()->GetWorldTimerManager().ClearTimer(ReplenishBulletTimerHandle);
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
	

	GetOwner()->GetWorldTimerManager().SetTimer(
		ReplenishBulletTimerHandle,
		this,
		&UCombatComponent::ReplenishBullet,
		BulletReplenishCooldown,
		true
	);
}

void UCombatComponent::ReplenishBullet()
{
	CurrentBullets = FMath::Min(CurrentBullets + 1, MaxBullets);
}

void UCombatComponent::OnTriggerStartAction(const FInputActionValue& Value)
{
	int32 BulletCost;
	switch (CurrentWeapon)
	{
	case EWeaponType::Rifle:
		BulletCost = RifleBulletCost;
		break;
	case EWeaponType::Shotgun:
		BulletCost = ShotgunBulletCost;
		break;
	}

	if (CurrentBullets < BulletCost)
	{
		if (OutOfBulletsSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), OutOfBulletsSound);
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
	CurrentCooldown = 0;

	// 1. Get the Enum as a UEnum pointer
	const UEnum* EnumPtr = StaticEnum<EWeaponType>();

	// 2. Convert the current value to a FString, then to TCHAR* for the log
	FString WeaponName = EnumPtr->GetValueAsString(CurrentWeapon);

	UE_LOG(LogTemp, Display, TEXT("Changed weapon to %s"), *WeaponName);
}

void UCombatComponent::OnTriggerAction(const FInputActionValue& Value)
{
	// Check if can fire
	if (CurrentCooldown > SMALL_NUMBER)
	{
		return;
	}

	// code duplication trigger warning:
	switch (CurrentWeapon)
	{
	case EWeaponType::Rifle:
		if (CurrentBullets < RifleBulletCost)
		{
			return;
		}
		PerformRifleShoot();
		CurrentBullets -= RifleBulletCost;

		if (RifleTriggerSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), RifleTriggerSound);
		}

		RifleRecoil();

		CurrentCooldown = 1.f / RifleFireRate;
		break;
	case EWeaponType::Shotgun:
		if (CurrentBullets < ShotgunBulletCost)
		{
			return;
		}

		PerformShotgunShoot();
		CurrentBullets -= ShotgunBulletCost;

		if (ShotgunTriggerSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ShotgunTriggerSound);
		}

		CurrentCooldown = 1.f / ShotgunFireRate;
		break;
	}
}

void UCombatComponent::PerformRifleShoot()
{
	FHitResult Hit;
	FVector TraceStart = Camera->GetComponentLocation();

	FRotator HorizontalRecoilRotator;
	HorizontalRecoilRotator.Yaw = CurrentOffset.X;

	FVector Direction = Camera->GetForwardVector();
	Direction = HorizontalRecoilRotator.RotateVector(Direction);

	FVector TraceEnd = TraceStart + Direction * RifleRange;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Pawn);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ShootingTargetChannel, QueryParams);

	if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
	{
		FTransform SpawnTransform((-Direction).Rotation(), TraceStart + Direction * 100.f);
		DrawDebugBox(GetWorld(), Hit.Location, FVector(5.f, 5.f, 5.f), FColor::Red, true, 1.f, 0, 1.f);
	}
}

void UCombatComponent::PerformShotgunShoot()
{
	FHitResult Hit;
	FVector TraceStart = Camera->GetComponentLocation();
	FVector Direction = Camera->GetForwardVector();
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Pawn);

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			FRotator ShotgunSpreadRotator;
			ShotgunSpreadRotator.Yaw = i * ShotgunSpreadAngle;
			ShotgunSpreadRotator.Pitch = j * ShotgunSpreadAngle;
			
			FVector BulletDirection = ShotgunSpreadRotator.RotateVector(Direction);
			FVector TraceEnd = TraceStart + BulletDirection * ShotgunRange;

			GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ShootingTargetChannel, QueryParams);

			if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
			{
				FTransform SpawnTransform((-Direction).Rotation(), TraceStart + Direction * 100.f);
				DrawDebugBox(GetWorld(), Hit.Location, FVector(5.f, 5.f, 5.f), FColor::Red, true, 1.f, 0, 1.f);
			}
		}
	}

}

void UCombatComponent::RifleRecoil()
{
	int Sign = FMath::RandBool() ? 1 : -1;
	FVector2D NextOffset = CurrentOffset + FVector2D(RifleRecoilAmount.X * Sign, RifleRecoilAmount.Y);
	NextOffset.X = FMath::Min(NextOffset.X, RifleMaxRecoilAmount.X);
	NextOffset.Y = FMath::Min(NextOffset.Y, RifleMaxRecoilAmount.Y);
	FVector2D DeltaOffset = NextOffset - CurrentOffset;
	Pawn->AddControllerPitchInput(-DeltaOffset.Y);
	CurrentOffset = NextOffset;
}

void UCombatComponent::RifleRecoverRecoil(float DeltaSeconds)
{
	float DeltaOffsetX = FMath::Min(CurrentOffset.X, RifleOffsetRecoverySpeed.X * DeltaSeconds);
	float DeltaOffsetY = FMath::Min(CurrentOffset.Y, RifleOffsetRecoverySpeed.Y * DeltaSeconds);
	CurrentOffset -= FVector2D(DeltaOffsetX, DeltaOffsetY);
	Pawn->AddControllerPitchInput(DeltaOffsetY);
}

void UCombatComponent::ShotgunRecoil()
{
	int Sign = FMath::RandBool() ? 1 : -1;
	FVector2D NextOffset = CurrentOffset + FVector2D(ShotgunRecoilAmount.X * Sign, ShotgunRecoilAmount.Y);
	NextOffset.X = FMath::Min(NextOffset.X, ShotgunMaxRecoilAmount.X);
	NextOffset.Y = FMath::Min(NextOffset.Y, ShotgunMaxRecoilAmount.Y);
	FVector2D DeltaOffset = NextOffset - CurrentOffset;
	Pawn->AddControllerPitchInput(-DeltaOffset.Y);
	CurrentOffset = NextOffset;
}

void UCombatComponent::ShotgunRecoverRecoil(float DeltaSeconds)
{
	float DeltaOffsetX = FMath::Min(CurrentOffset.X, ShotgunOffsetRecoverySpeed.X * DeltaSeconds);
	float DeltaOffsetY = FMath::Min(CurrentOffset.Y, ShotgunOffsetRecoverySpeed.Y * DeltaSeconds);
	CurrentOffset -= FVector2D(DeltaOffsetX, DeltaOffsetY);
	Pawn->AddControllerPitchInput(DeltaOffsetY);
}