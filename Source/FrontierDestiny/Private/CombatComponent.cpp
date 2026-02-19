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

	RecoverRecoil(DeltaTime);
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
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(
				ReloadAction,
				ETriggerEvent::Triggered,
				this,
				&UCombatComponent::OnReloadAction
			);
		}
	}
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentBullets = MaxBullets;
}

void UCombatComponent::OnTriggerStartAction(const FInputActionValue& Value)
{
	if (CurrentBullets == 0)
	{
		if (OutOfBulletsSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), OutOfBulletsSound);
		}
	}
}

void UCombatComponent::OnReloadAction(const FInputActionValue& Value)
{
	PerformReload();
}

void UCombatComponent::PerformReload()
{
	if (ReloadSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ReloadSound);
	}
	CurrentBullets = MaxBullets;
}

void UCombatComponent::OnTriggerAction(const FInputActionValue& Value)
{
	// Check if can fire
	if (CurrentCooldown > SMALL_NUMBER)
	{
		return;
	}

	// Check if out of bullets
	if (CurrentBullets == 0)
	{
		return;
	}

	PerformShoot();
	Recoil();

	CurrentBullets -= 1;

	if (TriggerSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), TriggerSound);
	}

	CurrentCooldown = 1.f / FireRate;
}

void UCombatComponent::PerformShoot()
{
	FHitResult Hit;
	FVector TraceStart = Camera->GetComponentLocation();

	FRotator HorizontalRecoilRotator;
	HorizontalRecoilRotator.Yaw = CurrentOffset.X;

	FVector Direction = Camera->GetForwardVector();
	Direction = HorizontalRecoilRotator.RotateVector(Direction);

	FVector TraceEnd = TraceStart + Direction * Range;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Pawn);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ShootingTargetChannel, QueryParams);
	// DrawDebugLine(GetWorld(), TraceStart, TraceEnd, Hit.bBlockingHit ? FColor::Blue : FColor::Red, false, 5.0f, 0, 1.0f);

	if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
	{
		FTransform SpawnTransform((-Direction).Rotation(), TraceStart + Direction * 100.f);

		/*ADamageNumber* NewDamageNumber = GetWorld()->SpawnActorDeferred<ADamageNumber>(
			ADamageNumber::StaticClass(),
			SpawnTransform,
			GetOwner(),
			Pawn,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (NewDamageNumber)
		{
			NewDamageNumber->DamageNumber = 50;
			NewDamageNumber->FinishSpawning(SpawnTransform);
		}*/

		DrawDebugBox(GetWorld(), Hit.Location, FVector(5.f, 5.f, 5.f), FColor::Red, true, 1.f, 0, 1.f);
	}
}

void UCombatComponent::Recoil()
{
	int Sign = FMath::RandBool() ? 1 : -1;
	FVector2D NextOffset = CurrentOffset + FVector2D(RecoilAmount.X * Sign, RecoilAmount.Y);
	NextOffset.X = FMath::Min(NextOffset.X, MaxRecoilAmount.X);
	NextOffset.Y = FMath::Min(NextOffset.Y, MaxRecoilAmount.Y);
	FVector2D DeltaOffset = NextOffset - CurrentOffset;
	// Pawn->AddControllerYawInput(-DeltaOffset.X);
	Pawn->AddControllerPitchInput(-DeltaOffset.Y);
	CurrentOffset = NextOffset;
}

void UCombatComponent::RecoverRecoil(float DeltaSeconds)
{
	float DeltaOffsetX = FMath::Min(CurrentOffset.X, OffsetRecoverySpeed.X * DeltaSeconds);
	float DeltaOffsetY = FMath::Min(CurrentOffset.Y, OffsetRecoverySpeed.Y * DeltaSeconds);
	CurrentOffset -= FVector2D(DeltaOffsetX, DeltaOffsetY);
	// Pawn->AddControllerYawInput(DeltaOffsetX);
	Pawn->AddControllerPitchInput(DeltaOffsetY);
}